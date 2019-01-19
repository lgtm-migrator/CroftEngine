#include "lightningball.h"

#include "engine/heightinfo.h"
#include "engine/laranode.h"
#include "level/level.h"

namespace engine
{
namespace items
{
namespace
{
gsl::not_null<std::shared_ptr<gameplay::Mesh>>
createBolt(uint16_t points, const gsl::not_null<std::shared_ptr<gameplay::ShaderProgram>>& program, GLfloat lineWidth)
{
    std::vector<glm::vec3> vertices( points );

    static const gameplay::gl::StructuredVertexBuffer::AttributeMapping attribs{
            {VERTEX_ATTRIBUTE_POSITION_NAME, gameplay::gl::VertexAttribute{
                    gameplay::gl::VertexAttribute::SingleAttribute<glm::vec3>{}}}
    };

    auto mesh = make_not_null_shared<gameplay::Mesh>( attribs, true );
    mesh->getBuffers()[0]->assign<glm::vec3>( gsl::make_not_null( &vertices[0] ), points );

    std::vector<uint16_t> indices;
    for( uint16_t i = 0; i < points; ++i )
        indices.emplace_back( i );

    gameplay::gl::VertexArrayBuilder builder;

    auto indexBuffer = make_not_null_shared<gameplay::gl::IndexBuffer>();
    indexBuffer->setData( indices, false );
    builder.attach( indexBuffer );
    builder.attach( mesh->getBuffers() );

    const auto part = make_not_null_shared<gameplay::MeshPart>( builder.build( program->getHandle() ), GL_LINE_STRIP );
    mesh->addPart( part );

    mesh->getRenderState().setLineSmooth( true );
    mesh->getRenderState().setLineWidth( lineWidth );

    auto material = std::make_shared<gameplay::Material>( program );
    material->getParameter( "u_modelViewMatrix" )->bindModelViewMatrix();
    material->getParameter( "u_projectionMatrix" )->bindProjectionMatrix();

    part->setMaterial( material );

    return mesh;
}

using Bolt = std::array<glm::vec3, LightningBall::SegmentPoints>;

Bolt updateBolt(glm::vec3 start, const glm::vec3& end, const gameplay::Mesh& mesh)
{
    const auto segmentSize = (end - start) / float( LightningBall::SegmentPoints );

    Bolt bolt;

    BOOST_ASSERT( mesh.getBuffers()[0]->getVertexCount() == LightningBall::SegmentPoints );
    auto boltData = mesh.getBuffers()[0]->mapTypedRw<glm::vec3>();
    for( size_t j = 0; j < LightningBall::SegmentPoints; j++ )
    {
        glm::vec3 buckling{
                util::rand15s( loader::QuarterSectorSize ),
                util::rand15s( loader::QuarterSectorSize ),
                util::rand15s( loader::QuarterSectorSize )
        };

        if( j == LightningBall::SegmentPoints - 1 )
            buckling.y = 0;

        bolt[j] = boltData[j] = start + buckling;
        start += segmentSize;
    }
    mesh.getBuffers()[0]->unmap();

    return bolt;
}
}

LightningBall::LightningBall(const gsl::not_null<level::Level*>& level,
                             const gsl::not_null<const loader::Room*>& room,
                             const loader::Item& item,
                             const loader::SkeletalModelType& animatedModel,
                             const gsl::not_null<std::shared_ptr<gameplay::ShaderProgram>>& boltProgram)
        : ModelItemNode{level, room, item, true, animatedModel}
{
    if( animatedModel.nMeshes >= 1 )
    {
        m_poles = static_cast<size_t>(animatedModel.nMeshes - 1);
    }

    for( size_t i = 1; i < getSkeleton()->getChildren().size(); ++i )
    {
        getSkeleton()->getChildren()[i]->setDrawable( nullptr );
        getSkeleton()->getChildren()[i]->setVisible( false );
    }

    m_mainBoltMesh = createBolt( SegmentPoints, boltProgram, 10 );
    auto node = make_not_null_shared<gameplay::Node>( "lightning-bolt-main" );
    node->setDrawable( m_mainBoltMesh );
    addChild( gsl::make_not_null( getSkeleton() ), node );

    for( auto& childBolt : m_childBolts )
    {
        childBolt.mesh = createBolt( SegmentPoints, boltProgram, 3 );

        node = make_not_null_shared<gameplay::Node>( "lightning-bolt-child" );
        node->setDrawable( childBolt.mesh );
        addChild( gsl::make_not_null( getSkeleton() ), node );
    }
}

void LightningBall::update()
{
    if( !m_state.updateActivationTimeout() )
    {
        m_chargeTimeout = 1;
        m_shooting = false;
        m_laraHit = false;
        if( getLevel().roomsAreSwapped )
            getLevel().swapAllRooms();

        deactivate();
        m_state.triggerState = TriggerState::Inactive;
        prepareRender();
        return;
    }

    prepareRender();

    if( --m_chargeTimeout > 0 )
        return;

    if( m_shooting != 0 )
    {
        m_shooting = false;
        m_chargeTimeout = 35 + util::rand15( 45 );
        m_laraHit = false;
        if( getLevel().roomsAreSwapped )
            getLevel().swapAllRooms();

        return;
    }

    m_shooting = true;
    m_chargeTimeout = 20;
    m_laraHit = false;

    const auto radius = m_poles == 0 ? loader::SectorSize : loader::SectorSize * 5 / 2;
    if( getLevel().m_lara->isNear( *this, radius ) )
    {
        // target at lara
        m_mainBoltEnd = (getLevel().m_lara->m_state.position.position - m_state.position.position).toRenderSystem();
        m_mainBoltEnd = glm::vec3( (-m_state.rotation).toMatrix() * glm::vec4( m_mainBoltEnd, 1.0f ) );

        getLevel().m_lara->m_state.health -= 400;
        getLevel().m_lara->m_state.is_hit = true;

        m_laraHit = true;
    }
    else if( m_poles == 0 )
    {
        // we don't have poles, so just shoot downwards
        m_mainBoltEnd = glm::vec3{0, 0, 0};
        const auto sector = gsl::make_not_null( level::Level::findRealFloorSector( m_state.position ) );
        m_mainBoltEnd.y = -HeightInfo::fromFloor( sector, m_state.position.position, getLevel().m_itemNodes ).y;
        m_mainBoltEnd.y -= m_state.position.position.toRenderSystem().y;
    }
    else
    {
        // select a random "pole"
        const auto itemSpheres = getSkeleton()->getBoneCollisionSpheres(
                m_state,
                *getSkeleton()->getInterpolationInfo( m_state ).getNearestFrame(),
                nullptr );
        m_mainBoltEnd = itemSpheres[util::rand15( itemSpheres.size() - 1 ) + 1].getPosition()
                        - m_state.position.position.toRenderSystem();
        m_mainBoltEnd = glm::vec3( (-m_state.rotation).toMatrix() * glm::vec4( m_mainBoltEnd, 1.0f ) );
    }

    for( auto& childBolt : m_childBolts )
    {
        childBolt.startIndex = util::rand15( SegmentPoints - 1 );
        childBolt.end = m_mainBoltEnd + glm::vec3(
                util::rand15s( loader::QuarterSectorSize ),
                0,
                util::rand15s( loader::QuarterSectorSize ) );
    }

    if( !getLevel().roomsAreSwapped )
        getLevel().swapAllRooms();

    playSoundEffect( TR1SoundId::Chatter );
}

void LightningBall::collide(LaraNode& lara, CollisionInfo& info)
{
    if( !m_laraHit )
        return;

    lara.hit_direction = static_cast<core::Axis>(util::rand15( 4 ));
    if( ++lara.hit_frame > 34 )
        lara.hit_frame = 34;
}

void LightningBall::prepareRender()
{
    ModelItemNode::update();

    if( m_shooting == 0 )
    {
        for( size_t i = 1; i < getSkeleton()->getChildren().size(); ++i )
        {
            getSkeleton()->getChildren()[i]->setVisible( false );
        }
        return;
    }

    for( size_t i = 1; i < getSkeleton()->getChildren().size(); ++i )
    {
        getSkeleton()->getChildren()[i]->setVisible( i - 1 >= m_poles );
    }

    const auto nearestFrame = getSkeleton()->getInterpolationInfo( m_state ).getNearestFrame();
    const auto segmentStart = glm::vec3(
            core::fromPackedAngles( nearestFrame->getAngleData()[0] ) * glm::vec4( nearestFrame->pos.toGl(), 1.0f ) );

    const Bolt mainBolt = updateBolt( segmentStart, m_mainBoltEnd, *m_mainBoltMesh );

    for( const auto& childBolt : m_childBolts )
    {
        updateBolt( mainBolt[childBolt.startIndex], childBolt.end, *childBolt.mesh );
    }
}
}
}
