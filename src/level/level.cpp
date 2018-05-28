#include "level.h"

#include "engine/laranode.h"
#include "render/textureanimator.h"
#include "tr1level.h"
#include "tr2level.h"
#include "tr3level.h"
#include "tr4level.h"
#include "tr5level.h"

#include "engine/items/bat.h"
#include "engine/items/block.h"
#include "engine/items/bridgeflat.h"
#include "engine/items/collapsiblefloor.h"
#include "engine/items/dart.h"
#include "engine/items/dartgun.h"
#include "engine/items/door.h"
#include "engine/items/slopedbridge.h"
#include "engine/items/stubitem.h"
#include "engine/items/swingingblade.h"
#include "engine/items/switch.h"
#include "engine/items/tallblock.h"
#include "engine/items/trapdoordown.h"
#include "engine/items/trapdoorup.h"
#include "engine/items/pickupitem.h"
#include "engine/items/underwaterswitch.h"
#include "engine/items/wolf.h"

#include "loader/converter.h"

#include "util/md5.h"

#include <yaml-cpp/yaml.h>

#include <boost/range/adaptors.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

using namespace level;

Level::~Level() = default;

constexpr uint16_t TrackActionMusic = 13;
// welcome to my home, I*ll take you on a guided tour
constexpr uint16_t TrackWelcomeToMyHome = 29;
// with the walk button down, I won't fall off, even if you try to make me. go on, try it.
constexpr uint16_t TrackWalkWontFallOff = 37;
// then let go
constexpr uint16_t TrackThenLetGo = 49;
// let's go for a swim
constexpr uint16_t TrackLetsGoForASwim = 50;

/// \brief reads the mesh data.
void Level::readMeshData(loader::io::SDLReader& reader)
{
    const auto meshDataWords = reader.readU32();
    const auto basePos = reader.tell();

    const auto meshDataSize = meshDataWords * 2;
    reader.skip( meshDataSize );

    reader.readVector( m_meshIndices, reader.readU32() );
    const auto endPos = reader.tell();

    m_meshes.clear();

    uint32_t meshDataPos = 0;
    for( uint32_t i = 0; i < m_meshIndices.size(); i++ )
    {
        replace( m_meshIndices.begin(), m_meshIndices.end(), meshDataPos, i );

        reader.seek( basePos + std::streamoff( meshDataPos ) );

        if( gameToEngine( m_gameVersion ) >= Engine::TR4 )
            m_meshes.emplace_back( *loader::Mesh::readTr4( reader ) );
        else
            m_meshes.emplace_back( *loader::Mesh::readTr1( reader ) );

        for( auto pos : m_meshIndices )
        {
            if( pos > meshDataPos )
            {
                meshDataPos = pos;
                break;
            }
        }
    }

    reader.seek( endPos );
}

std::unique_ptr<Level> Level::createLoader(const std::string& filename, Game gameVersion, sol::state&& scriptEngine)
{
    std::string sfxPath = (boost::filesystem::path( filename ).remove_filename() / "MAIN.SFX").string();

    loader::io::SDLReader reader( filename );
    if( !reader.isOpen() )
        return nullptr;

    if( gameVersion == Game::Unknown )
        gameVersion = probeVersion( reader, filename );
    if( gameVersion == Game::Unknown )
        return nullptr;

    reader.seek( 0 );
    return createLoader( std::move( reader ), gameVersion, sfxPath, std::move( scriptEngine ) );
}

/** \brief reads the level.
  *
  * Takes a SDL_RWop and the game_version of the file and reads the structures into the members of TR_Level.
  */
std::unique_ptr<Level>
Level::createLoader(loader::io::SDLReader&& reader, Game game_version, const std::string& sfxPath,
                    sol::state&& scriptEngine)
{
    if( !reader.isOpen() )
        return nullptr;

    std::unique_ptr<Level> result;

    switch( game_version )
    {
        case Game::TR1:
            result = std::make_unique<level::TR1Level>( game_version, std::move( reader ), std::move( scriptEngine ) );
            break;
        case Game::TR1Demo:
        case Game::TR1UnfinishedBusiness:
            result = std::make_unique<level::TR1Level>( game_version, std::move( reader ), std::move( scriptEngine ) );
            result->m_demoOrUb = true;
            break;
        case Game::TR2:
            result = std::make_unique<level::TR2Level>( game_version, std::move( reader ), std::move( scriptEngine ) );
            break;
        case Game::TR2Demo:
            result = std::make_unique<level::TR2Level>( game_version, std::move( reader ), std::move( scriptEngine ) );
            result->m_demoOrUb = true;
            break;
        case Game::TR3:
            result = std::make_unique<level::TR3Level>( game_version, std::move( reader ), std::move( scriptEngine ) );
            break;
        case Game::TR4:
        case Game::TR4Demo:
            result = std::make_unique<level::TR4Level>( game_version, std::move( reader ), std::move( scriptEngine ) );
            break;
        case Game::TR5:
            result = std::make_unique<level::TR5Level>( game_version, std::move( reader ), std::move( scriptEngine ) );
            break;
        default:
            BOOST_THROW_EXCEPTION( std::runtime_error( "Invalid game version" ) );
    }

    result->m_sfxPath = sfxPath;
    return result;
}

Game Level::probeVersion(loader::io::SDLReader& reader, const std::string& filename)
{
    if( !reader.isOpen() || filename.length() < 5 )
        return Game::Unknown;

    std::string ext;
    ext += filename[filename.length() - 4];
    ext += (char)toupper( filename[filename.length() - 3] );
    ext += (char)toupper( filename[filename.length() - 2] );
    ext += (char)toupper( filename[filename.length() - 1] );

    reader.seek( 0 );
    uint8_t check[4];
    reader.readBytes( check, 4 );

    Game ret = Game::Unknown;
    if( ext == ".PHD" )
    {
        if( check[0] == 0x20 && check[1] == 0x00 && check[2] == 0x00 && check[3] == 0x00 )
        {
            ret = Game::TR1;
        }
    }
    else if( ext == ".TUB" )
    {
        if( check[0] == 0x20 && check[1] == 0x00 && check[2] == 0x00 && check[3] == 0x00 )
        {
            ret = Game::TR1UnfinishedBusiness;
        }
    }
    else if( ext == ".TR2" )
    {
        if( check[0] == 0x2D && check[1] == 0x00 && check[2] == 0x00 && check[3] == 0x00 )
        {
            ret = Game::TR2;
        }
        else if( (check[0] == 0x38 || check[0] == 0x34) && check[1] == 0x00 && (check[2] == 0x18 || check[2] == 0x08)
                 && check[3] == 0xFF )
        {
            ret = Game::TR3;
        }
    }
    else if( ext == ".TR4" )
    {
        if( check[0] == 0x54 && // T
            check[1] == 0x52 && // R
            check[2] == 0x34 && // 4
            check[3] == 0x00 )
        {
            ret = Game::TR4;
        }
        else if( check[0] == 0x54 && // T
                 check[1] == 0x52 && // R
                 check[2] == 0x34 && // 4
                 check[3] == 0x63 ) //
        {
            ret = Game::TR4;
        }
        else if( check[0] == 0xF0 && // T
                 check[1] == 0xFF && // R
                 check[2] == 0xFF && // 4
                 check[3] == 0xFF )
        {
            ret = Game::TR4;
        }
    }
    else if( ext == ".TRC" )
    {
        if( check[0] == 0x54 && // T
            check[1] == 0x52 && // R
            check[2] == 0x34 && // C
            check[3] == 0x00 )
        {
            ret = Game::TR5;
        }
    }

    return ret;
}

const loader::StaticMesh* Level::findStaticMeshById(uint32_t meshId) const
{
    for( const auto& mesh : m_staticMeshes )
        if( mesh.id == meshId )
            return &mesh;

    return nullptr;
}

int Level::findStaticMeshIndexById(uint32_t meshId) const
{
    for( const auto& mesh : m_staticMeshes )
    {
        if( mesh.id == meshId )
        {
            BOOST_ASSERT( mesh.mesh < m_meshIndices.size() );
            return m_meshIndices[mesh.mesh];
        }
    }

    return -1;
}

const std::unique_ptr<loader::SkeletalModelType>& Level::findAnimatedModelForType(uint32_t type) const
{
    const auto it = m_animatedModels.find(type);
    if (it != m_animatedModels.end())
        return it->second;

    static const std::unique_ptr<loader::SkeletalModelType> none;
    return none;
}

boost::optional<size_t> Level::findSpriteSequenceForType(uint32_t type) const
{
    for( size_t i = 0; i < m_spriteSequences.size(); i++ )
        if( m_spriteSequences[i].type == type )
            return i;

    return boost::none;
}

std::vector<std::shared_ptr<gameplay::gl::Texture>>
Level::createTextures(loader::trx::Glidos* glidos, const boost::filesystem::path& lvlName)
{
    BOOST_ASSERT( !m_textures.empty() );
    std::vector<std::shared_ptr<gameplay::gl::Texture>> textures;
    for( auto& texture : m_textures )
    {
        textures.emplace_back( texture.toTexture( glidos, lvlName ) );
    }
    return textures;
}

std::map<loader::TextureLayoutProxy::TextureKey, std::shared_ptr<gameplay::Material>>
Level::createMaterials(const std::vector<std::shared_ptr<gameplay::gl::Texture>>& textures,
                       const std::shared_ptr<gameplay::ShaderProgram>& shader)
{
    const auto texMask = gameToEngine( m_gameVersion ) == Engine::TR4 ? loader::TextureIndexMaskTr4
                                                                      : loader::TextureIndexMask;
    std::map<loader::TextureLayoutProxy::TextureKey, std::shared_ptr<gameplay::Material>> materials;
    for( loader::TextureLayoutProxy& proxy : m_textureProxies )
    {
        const auto& key = proxy.textureKey;
        if( materials.find( key ) != materials.end() )
            continue;

        materials[key] = proxy.createMaterial( textures[key.tileAndFlag & texMask], shader );
    }
    return materials;
}

engine::LaraNode* Level::createItems(const std::vector<std::shared_ptr<gameplay::gl::Texture>>& textures)
{
    auto spriteMaterial = std::make_shared<gameplay::Material>( "shaders/textured_2.vert",
                                                                "shaders/textured_2.frag" );
    spriteMaterial->getStateBlock().setCullFace( false );

#if 1
    spriteMaterial->getParameter( "u_modelViewMatrix" )
                  ->bind( [](const gameplay::Node& node, gameplay::gl::Program::ActiveUniform& uniform) {
                      auto m = node.getModelViewMatrix();
                      // clear out rotation component
                      for( int i : {0, 2} )
                          for( int j = 0; j < 3; ++j )
                              m[i][j] = i == j ? 1 : 0;

                      uniform.set( m );
                  } );
#else
    spriteMaterial->getParameter("u_modelViewMatrix")->bindModelViewMatrix();
#endif

    spriteMaterial->getParameter( "u_modelMatrix" )->bindModelMatrix();
    spriteMaterial->getParameter( "u_projectionMatrix" )->bindProjectionMatrix();

    spriteMaterial->getParameter( "u_baseLightDiff" )->set( 0.0f );
    spriteMaterial->getParameter( "u_lightPosition" )->set( glm::vec3{std::numeric_limits<float>::quiet_NaN()} );

    engine::LaraNode* lara = nullptr;
    int id = -1;
    for( loader::Item& item : m_items )
    {
        ++id;

        BOOST_ASSERT( item.room < m_rooms.size() );
        loader::Room& room = m_rooms[item.room];

        if( const auto& model = findAnimatedModelForType( item.type ) )
        {
            std::shared_ptr<engine::items::ItemNode> modelNode;

            if( item.type == 0 )
            {
                modelNode = createSkeletalModel<engine::LaraNode>( id, *model, &room, item );
                lara = static_cast<engine::LaraNode*>(modelNode.get());
            }
            else if( auto objectInfo = m_scriptEngine["getObjectInfo"].call( item.type + 9999999 ) )
            {
                BOOST_LOG_TRIVIAL(info) << "Instantiating scripted type " << item.type << "/id " << id;

                modelNode = std::make_shared<engine::items::ScriptedItem>( this,
                                                                           "skeleton:" + std::to_string( id ) + "(type:" + std::to_string( item.type ) + ")",
                                                                           &room,
                                                                           item,
                                                                           *model,
                                                                           objectInfo);
                for( size_t boneIndex = 0; boneIndex < model->nmeshes; ++boneIndex )
                {
                    BOOST_ASSERT( model->frame_number + boneIndex < m_meshIndices.size() );
                    auto node = std::make_shared<gameplay::Node>(
                            modelNode->getNode()->getId() + "/bone:" + std::to_string( boneIndex ) );
                    node->setDrawable( m_models[m_meshIndices[model->frame_number + boneIndex]] );
                    modelNode->getNode()->addChild( node );
                }

                BOOST_ASSERT( modelNode->getNode()->getChildCount() == model->nmeshes );
            }
            else if( item.type == 7 )
            {
                modelNode = createSkeletalModel<engine::items::Wolf>( id, *model, &room, item );
            }
            else if( item.type == 9 )
            {
                modelNode = createSkeletalModel<engine::items::Bat>( id, *model, &room, item );
            }
            else if( item.type == 35 )
            {
                modelNode = createSkeletalModel<engine::items::CollapsibleFloor>( id, *model, &room, item );
            }
            else if( item.type == 36 )
            {
                modelNode = createSkeletalModel<engine::items::SwingingBlade>( id, *model, &room, item );
            }
            else if( item.type == 39 )
            {
                modelNode = createSkeletalModel<engine::items::Dart>( id, *model, &room, item );
            }
            else if( item.type == 40 )
            {
                modelNode = createSkeletalModel<engine::items::DartGun>( id, *model, &room, item );
            }
            else if( item.type == 41 )
            {
                modelNode = createSkeletalModel<engine::items::TrapDoorUp>( id, *model, &room, item );
            }
            else if( item.type >= 48 && item.type <= 51 )
            {
                modelNode = createSkeletalModel<engine::items::Block>( id, *model, &room, item );
            }
            else if( item.type == 52 )
            {
                modelNode = createSkeletalModel<engine::items::TallBlock>( id, *model, &room, item );
            }
            else if( item.type == 55 )
            {
                modelNode = createSkeletalModel<engine::items::Switch>( id, *model, &room, item );
            }
            else if( item.type == 56 )
            {
                modelNode = createSkeletalModel<engine::items::UnderwaterSwitch>( id, *model, &room, item );
            }
            else if( item.type >= 57 && item.type <= 64 )
            {
                modelNode = createSkeletalModel<engine::items::Door>( id, *model, &room, item );
            }
            else if( item.type >= 65 && item.type <= 66 )
            {
                modelNode = createSkeletalModel<engine::items::TrapDoorDown>( id, *model, &room, item );
            }
            else if( item.type == 68 )
            {
                modelNode = createSkeletalModel<engine::items::BridgeFlat>( id, *model, &room, item );
            }
            else if( item.type == 69 )
            {
                modelNode = createSkeletalModel<engine::items::BridgeSlope1>( id, *model, &room, item );
            }
            else if( item.type == 70 )
            {
                modelNode = createSkeletalModel<engine::items::BridgeSlope2>( id, *model, &room, item );
            }
            else if( item.type == 141 || item.type == 142 || item.type == 129 || item.type == 130 || item.type == 131
                     || item.type == 132 || item.type == 110 || item.type == 111 || item.type == 112 || item.type == 113
                     || item.type == 84 || item.type == 85 || item.type == 86 || item.type == 87 || item.type == 88
                     || item.type == 89 || item.type == 90 || item.type == 91 || item.type == 92 || item.type == 93
                     || item.type == 94 || item.type == 144 || item.type == 126 )
            {
                modelNode = createSkeletalModel<engine::items::PickupItem>( id, *model, &room, item );
            }
            else
            {
                modelNode = createSkeletalModel<engine::items::StubItem>( id, *model, &room, item );
            }

            m_itemNodes[id] = modelNode;
            room.node->addChild( modelNode->getNode() );

            modelNode->applyTransform();
            modelNode->updateLighting();

            continue;
        }

        if( const auto sequenceId = findSpriteSequenceForType( item.type ) )
        {
            BOOST_ASSERT( !findAnimatedModelForType( item.type ) );
            BOOST_ASSERT( *sequenceId < m_spriteSequences.size() );
            const loader::SpriteSequence& spriteSequence = m_spriteSequences[*sequenceId];

            BOOST_ASSERT( spriteSequence.offset < m_sprites.size() );

            const loader::Sprite& sprite = m_sprites[spriteSequence.offset];

            auto node = std::make_shared<engine::items::SpriteItemNode>( this,
                                                                         "sprite:" + std::to_string( id ) + "(type:"
                                                                         + std::to_string( item.type ) + ")",
                                                                         &room,
                                                                         item,
                                                                         true,
                                                                         sprite,
                                                                         spriteMaterial,
                                                                         textures );

            m_itemNodes[id] = node;
            room.node->addChild( node->getNode() );

            node->applyTransform();

            continue;
        }

        BOOST_LOG_TRIVIAL( error ) << "Failed to find an appropriate animated model for item " << id << "/type "
                                   << int( item.type );
    }

    return lara;
}

template<typename T>
std::shared_ptr<T> Level::createSkeletalModel(size_t id,
                                              const loader::SkeletalModelType& model,
                                              const gsl::not_null<const loader::Room*>& room,
                                              const loader::Item& item)
{
    static_assert( std::is_base_of<engine::items::ItemNode, T>::value, "T must be derived from engine::ItemNode" );

    if( model.anim_index == 0xffff )
    {
        BOOST_LOG_TRIVIAL( error ) << "Model 0x" << std::hex << reinterpret_cast<uintptr_t>(&model) << std::dec
                                   << " has animationIndex==0xffff";
        return nullptr;
    }

    auto skeletalModel = std::make_shared<T>( this,
                                              "skeleton:" + std::to_string( id ) + "(type:" + std::to_string( item.type )
                                              + ")",
                                              room,
                                              item,
                                              model );
    for( size_t boneIndex = 0; boneIndex < model.nmeshes; ++boneIndex )
    {
        BOOST_ASSERT( model.frame_number + boneIndex < m_meshIndices.size() );
        auto node = std::make_shared<gameplay::Node>(
                skeletalModel->getNode()->getId() + "/bone:" + std::to_string( boneIndex ) );
        node->setDrawable( m_models[m_meshIndices[model.frame_number + boneIndex]] );
        skeletalModel->getNode()->addChild( node );
    }

    BOOST_ASSERT( skeletalModel->getNode()->getChildCount() == model.nmeshes );

    skeletalModel->getSkeleton()->updatePose(skeletalModel->m_state);

    return skeletalModel;
}

YAML::Node
parseCommandSequence(const uint16_t*& rawFloorData, const engine::floordata::SequenceCondition sequenceCondition)
{
    YAML::Node sequence;
    const uint16_t activationRequestRaw{*rawFloorData++};
    const engine::floordata::ActivationState activationRequest{activationRequestRaw};
    for( size_t i = 0; i < 5; ++i )
    {
        if( activationRequest.isInActivationSet( i ) )
            sequence["activationBits"].push_back( i );
    }
    sequence["timeout"] = engine::floordata::ActivationState::extractTimeout( activationRequestRaw );
    sequence["oneshot"] = activationRequest.isOneshot();
    sequence["locked"] = activationRequest.isLocked();
    sequence["inverted"] = activationRequest.isInverted();

    switch( sequenceCondition )
    {
        case engine::floordata::SequenceCondition::LaraIsHere:
            sequence["if"] = "laraIsHere";
            break;
        case engine::floordata::SequenceCondition::LaraOnGround:
            sequence["if"] = "laraOnGround";
            break;
        case engine::floordata::SequenceCondition::ItemActivated:
        {
            const engine::floordata::Command commandHeader{*rawFloorData++};
            sequence["if"] = "itemActivated";
            sequence["itemId"] = commandHeader.parameter;
        }
            break;
        case engine::floordata::SequenceCondition::KeyUsed:
        {
            const engine::floordata::Command commandHeader{*rawFloorData++};
            sequence["if"] = "keyUsed";
            sequence["itemId"] = commandHeader.parameter;
        }
            break;
        case engine::floordata::SequenceCondition::ItemPickedUp:
        {
            const engine::floordata::Command commandHeader{*rawFloorData++};
            sequence["if"] = "itemPickedUp";
            sequence["itemId"] = commandHeader.parameter;
        }
            break;
        case engine::floordata::SequenceCondition::ItemIsHere:
            sequence["if"] = "itemIsHere";
            break;
        case engine::floordata::SequenceCondition::LaraOnGroundInverted:
            sequence["if"] = "laraOnGroundInverted";
            break;
        case engine::floordata::SequenceCondition::LaraInCombatMode:
            sequence["if"] = "laraInCombatMode";
            break;
        case engine::floordata::SequenceCondition::Dummy:
            sequence["if"] = "dummy";
            break;
        case engine::floordata::SequenceCondition::AntiTrigger:
            sequence["if"] = "antiTrigger";
            break;
        case engine::floordata::SequenceCondition::HeavySwitch:
            sequence["if"] = "heavySwitch";
            break;
        case engine::floordata::SequenceCondition::HeavyAntiTrigger:
            sequence["if"] = "heavyAntiTrigger";
            break;
        case engine::floordata::SequenceCondition::Monkey:
            sequence["if"] = "monkey";
            break;
        case engine::floordata::SequenceCondition::Skeleton:
            sequence["if"] = "skeleton";
            break;
        case engine::floordata::SequenceCondition::TightRope:
            sequence["if"] = "tightRope";
            break;
        case engine::floordata::SequenceCondition::CrawlDuck:
            sequence["if"] = "crawlDuck";
            break;
        case engine::floordata::SequenceCondition::Climb:
            sequence["if"] = "climb";
            break;
        default:
            sequence["if"] = "true(" + std::to_string( static_cast<int>(sequenceCondition) ) + ")";
    }

    while( true )
    {
        const engine::floordata::Command commandHeader{*rawFloorData++};

        YAML::Node commandTree;

        switch( commandHeader.opcode )
        {
            case engine::floordata::CommandOpcode::Activate:
                commandTree["opcode"] = "activate";
                commandTree["itemId"] = commandHeader.parameter;
                break;
            case engine::floordata::CommandOpcode::SwitchCamera:
            {
                const engine::floordata::CameraParameters camParams{*rawFloorData++};
                commandTree["opcode"] = "switchCamera";
                commandTree["cameraId"] = commandHeader.parameter;
                commandTree["duration"] = int( camParams.timeout );
                commandTree["onlyOnce"] = camParams.oneshot;
                commandTree["smoothness"] = int( camParams.smoothness );
                commandHeader.isLast = camParams.isLast;
            }
                break;
            case engine::floordata::CommandOpcode::UnderwaterCurrent:
                commandTree["opcode"] = "underwaterFlow";
                break;
            case engine::floordata::CommandOpcode::FlipMap:
                commandTree["opcode"] = "flipMap";
                commandTree["maskId"] = commandHeader.parameter;
                break;
            case engine::floordata::CommandOpcode::FlipOn:
                commandTree["opcode"] = "flipOn";
                commandTree["maskId"] = commandHeader.parameter;
                break;
            case engine::floordata::CommandOpcode::FlipOff:
                commandTree["opcode"] = "flipOff";
                commandTree["maskId"] = commandHeader.parameter;
                break;
            case engine::floordata::CommandOpcode::LookAt:
                commandTree["opcode"] = "lookAt";
                commandTree["itemId"] = commandHeader.parameter;
                break;
            case engine::floordata::CommandOpcode::EndLevel:
                commandTree["opcode"] = "endLevel";
                break;
            case engine::floordata::CommandOpcode::PlayTrack:
                commandTree["opcode"] = "playTrack";
                commandTree["track"] = commandHeader.parameter;
                break;
            case engine::floordata::CommandOpcode::FlipEffect:
                commandTree["opcode"] = "flipEffect";
                commandTree["effect"] = commandHeader.parameter;
                break;
            case engine::floordata::CommandOpcode::Secret:
                commandTree["opcode"] = "secret";
                commandTree["secretId"] = commandHeader.parameter;
                break;
            case engine::floordata::CommandOpcode::ClearBodies:
                commandTree["opcode"] = "clearBodies";
                commandTree["target"] = commandHeader.parameter;
                break;
            case engine::floordata::CommandOpcode::FlyBy:
                commandTree["opcode"] = "flyby";
                commandTree["target"] = commandHeader.parameter;
                break;
            case engine::floordata::CommandOpcode::CutScene:
                commandTree["opcode"] = "cutScene";
                commandTree["target"] = commandHeader.parameter;
                break;
            default:
                BOOST_ASSERT( false );
        }

        sequence["commands"].push_back( commandTree );

        if( commandHeader.isLast )
            break;
    }

    return sequence;
}

void Level::setUpRendering(gameplay::Game* game,
                           const boost::filesystem::path& assetPath,
                           const boost::filesystem::path& lvlName,
                           const std::unique_ptr<loader::trx::Glidos>& glidos)
{
    m_inputHandler = std::make_unique<engine::InputHandler>( game->getWindow() );

    auto textures = createTextures( glidos.get(), lvlName );

    auto texturedShader = gameplay::ShaderProgram::createFromFile( "shaders/textured_2.vert",
                                                                   "shaders/textured_2.frag" );
    std::map<loader::TextureLayoutProxy::TextureKey, std::shared_ptr<gameplay::Material>> materials = createMaterials(
            textures, texturedShader );

    std::shared_ptr<gameplay::Material> colorMaterial = std::make_shared<gameplay::Material>( "shaders/colored_2.vert",
                                                                                              "shaders/colored_2.frag" );
    colorMaterial->getParameter( "u_modelMatrix" )->bindModelMatrix();
    colorMaterial->getParameter( "u_modelViewMatrix" )->bindModelViewMatrix();
    colorMaterial->getParameter( "u_projectionMatrix" )->bindProjectionMatrix();

    m_textureAnimator = std::make_shared<render::TextureAnimator>( m_animatedTextures );

    for( auto& mesh : m_meshes )
    {
        m_models.emplace_back( mesh.createModel( m_textureProxies, materials, colorMaterial, *m_palette,
                                                 *m_textureAnimator ) );
    }

    game->getScene()->setActiveCamera(
            std::make_shared<gameplay::Camera>( glm::radians( 80.0f ), game->getAspectRatio(), 10.0f, 20480.0f ) );

    auto waterTexturedShader = gameplay::ShaderProgram::createFromFile( "shaders/textured_2.vert",
                                                                        "shaders/textured_2.frag",
                                                                        {"WATER"} );
    std::map<loader::TextureLayoutProxy::TextureKey, std::shared_ptr<gameplay::Material>> waterMaterials = createMaterials(
            textures, waterTexturedShader );

    for( size_t i = 0; i < m_rooms.size(); ++i )
    {
        m_rooms[i].createSceneNode( i, *this, textures, materials, waterMaterials, m_models, *m_textureAnimator );
        game->getScene()->addNode( m_rooms[i].node );
    }

    {
        loader::Converter objWriter{assetPath / lvlName};

        for( size_t i = 0; i < m_textures.size(); ++i )
        {
            objWriter.write( m_textures[i].toImage( nullptr, {} ), i );
        }

        for( const auto& trModel : m_animatedModels | boost::adaptors::map_values )
        {
            for( size_t boneIndex = 0; boneIndex < trModel->nmeshes; ++boneIndex )
            {
                BOOST_ASSERT( trModel->frame_number + boneIndex < m_meshIndices.size() );
                BOOST_ASSERT( m_meshIndices[trModel->frame_number + boneIndex] < m_models.size() );

                std::string filename = "model_" + std::to_string( trModel->typeId ) + "_" + std::to_string( boneIndex )
                                       + ".dae";
                if( !objWriter.exists( filename ) )
                {
                    BOOST_LOG_TRIVIAL( info ) << "Saving model " << filename;

                    const auto& model = m_models[m_meshIndices[trModel->frame_number + boneIndex]];
                    objWriter.write( model, filename, materials, {}, glm::vec3( 0.8f ) );
                }

                filename = "model_override_" + std::to_string( trModel->typeId ) + "_" + std::to_string( boneIndex )
                           + ".dae";
                if( objWriter.exists( filename ) )
                {
                    BOOST_LOG_TRIVIAL( info ) << "Loading override model " << filename;

                    m_models[m_meshIndices[trModel->frame_number + boneIndex]] = objWriter
                            .readModel( filename, texturedShader, glm::vec3( 0.8f ) );
                }
            }
        }

        for( size_t i = 0; i < m_rooms.size(); ++i )
        {
            auto& room = m_rooms[i];

            std::string filename = "room_" + std::to_string( i ) + ".dae";
            if( !objWriter.exists( filename ) )
            {
                BOOST_LOG_TRIVIAL( info ) << "Saving room model " << filename;

                const auto drawable = room.node->getDrawable();
                const auto model = std::dynamic_pointer_cast<gameplay::Model>( drawable );
                BOOST_ASSERT( model != nullptr );
                objWriter.write( model, filename, materials, waterMaterials, glm::vec3{room.getAmbientBrightness()} );

                filename = "room_" + std::to_string( i ) + ".yaml";
                BOOST_LOG_TRIVIAL( info ) << "Saving floor data to " << filename;

                YAML::Node floorDataTree;
                for( size_t x = 0; x < room.sectorCountX; ++x )
                {
                    for( size_t z = 0; z < room.sectorCountZ; ++z )
                    {
                        const gsl::not_null<const loader::Sector*> sector = room
                                .getSectorByIndex( gsl::narrow<int>( x ), gsl::narrow<int>( z ) );
                        YAML::Node sectorTree;
                        sectorTree["position"]["x"] = x;
                        sectorTree["position"]["z"] = z;
                        if( sector->floorHeight != -127 )
                            sectorTree["layout"]["floor"] = sector->floorHeight * loader::QuarterSectorSize
                                                            - room.position.Y;
                        if( sector->ceilingHeight != -127 )
                            sectorTree["layout"]["ceiling"] = sector->ceilingHeight * loader::QuarterSectorSize
                                                              - room.position.Y;
                        if( sector->roomIndexBelow != 0xff )
                            sectorTree["relations"]["roomBelow"] = int( sector->roomIndexBelow );
                        if( sector->roomIndexAbove != 0xff )
                            sectorTree["relations"]["roomAbove"] = int( sector->roomIndexAbove );
                        if( sector->boxIndex >= 0 )
                            sectorTree["relations"]["box"] = sector->boxIndex;

                        const uint16_t* rawFloorData = &m_floorData[sector->floorDataIndex];
                        while( true )
                        {
                            const engine::floordata::FloorDataChunk chunkHeader{*rawFloorData++};
                            switch( chunkHeader.type )
                            {
                                case engine::floordata::FloorDataChunkType::FloorSlant:
                                    sectorTree["layout"]["floorSlant"]["x"] =
                                            gsl::narrow_cast<int8_t>( *rawFloorData & 0xff ) + 0;
                                    sectorTree["layout"]["floorSlant"]["z"] =
                                            gsl::narrow_cast<int8_t>( (*rawFloorData >> 8) & 0xff ) + 0;
                                    ++rawFloorData;
                                    break;
                                case engine::floordata::FloorDataChunkType::CeilingSlant:
                                    sectorTree["layout"]["ceilingSlant"]["x"] =
                                            gsl::narrow_cast<int8_t>( *rawFloorData & 0xff ) + 0;
                                    sectorTree["layout"]["ceilingSlant"]["z"] =
                                            gsl::narrow_cast<int8_t>( (*rawFloorData >> 8) & 0xff ) + 0;
                                    ++rawFloorData;
                                    break;
                                case engine::floordata::FloorDataChunkType::PortalSector:
                                    sectorTree["relations"]["portalToRoom"] = (*rawFloorData & 0xff);
                                    ++rawFloorData;
                                    break;
                                case engine::floordata::FloorDataChunkType::Death:
                                    sectorTree["characteristics"].push_back( "deadly" );
                                    break;
                                case engine::floordata::FloorDataChunkType::CommandSequence:
                                    sectorTree["sequences"].push_back(
                                            parseCommandSequence( rawFloorData, chunkHeader.sequenceCondition ) );
                                    break;
                                default:
                                    break;
                            }
                            if( chunkHeader.isLast )
                                break;
                        }

                        if( sectorTree.size() > 2 ) // only emit if we have more information than x/y coordinates
                            floorDataTree["sectors"].push_back( sectorTree );
                    }
                }

                objWriter.write( filename, floorDataTree );
            }

            filename = "room_override_" + std::to_string( i ) + ".dae";
            if( !objWriter.exists( filename ) )
                continue;

            BOOST_LOG_TRIVIAL( info ) << "Loading room override model " << filename;

            room.node->setDrawable( nullptr );

            auto model = objWriter.readModel( filename, room.isWaterRoom() ? waterTexturedShader : texturedShader,
                                              glm::vec3( room.getAmbientBrightness() ) );
            room.node->setDrawable( model );
        }

        BOOST_LOG_TRIVIAL( info ) << "Saving full level to _level.dae";
        objWriter.write( m_rooms, "_level.dae", materials, waterMaterials );
    }

    m_lara = createItems( textures );
    if( m_lara == nullptr )
        return;

    m_cameraController = new engine::CameraController( this, m_lara, game->getScene()->getActiveCamera() );

    for( const loader::SoundSource& src : m_soundSources )
    {
        auto handle = playSound( src.sound_id, src.position.toRenderSystem() );
        handle->setLooping( true );
        m_audioDev.registerSource( handle );
    }
}

void Level::convertTexture(loader::ByteTexture& tex, loader::Palette& pal, loader::DWordTexture& dst)
{
    for( int y = 0; y < 256; y++ )
    {
        for( int x = 0; x < 256; x++ )
        {
            int col = tex.pixels[y][x];

            if( col > 0 )
                dst.pixels[y][x] = {pal.color[col].r, pal.color[col].g, pal.color[col].b, 255};
            else
                dst.pixels[y][x] = {0, 0, 0, 0};
        }
    }

    dst.md5 = util::md5( &tex.pixels[0][0], 256 * 256 );
}

void Level::convertTexture(loader::WordTexture& tex, loader::DWordTexture& dst)
{
    for( int y = 0; y < 256; y++ )
    {
        for( int x = 0; x < 256; x++ )
        {
            int col = tex.pixels[y][x];

            if( (col & 0x8000) != 0 )
            {
                const uint8_t r = static_cast<const uint8_t>((col & 0x00007c00) >> 7);
                const uint8_t g = static_cast<const uint8_t>((col & 0x000003e0) >> 2);
                const uint8_t b = static_cast<const uint8_t>((col & 0x0000001f) << 3);
                dst.pixels[y][x] = {r, g, b, 1};
            }
            else
            {
                dst.pixels[y][x] = {0, 0, 0, 0};
            }
        }
    }
}

gsl::not_null<const loader::Sector*> Level::findRealFloorSector(const core::TRCoordinates& position,
                                                                const loader::Room** room) const
{
    const loader::Sector* sector = nullptr;
    while( true )
    {
        sector = (*room)->findFloorSectorWithClampedIndex( (position.X - (*room)->position.X) / loader::SectorSize,
                                                           (position.Z - (*room)->position.Z) / loader::SectorSize );
        Expects( sector != nullptr );
        const auto portalTarget = engine::floordata::getPortalTarget( m_floorData, sector->floorDataIndex );
        if( !portalTarget )
        {
            break;
        }

        BOOST_ASSERT( *portalTarget != 0xff && *portalTarget < m_rooms.size() );
        *room = &m_rooms[*portalTarget];
    }

    Expects( sector != nullptr );
    if( sector->floorHeight * loader::QuarterSectorSize > position.Y )
    {
        while( sector->ceilingHeight * loader::QuarterSectorSize >= position.Y && sector->roomAbove != nullptr )
        {
            *room = sector->roomAbove;
            sector = (*room)->getSectorByAbsolutePosition( position );
            Expects( sector != nullptr );
        }
    }
    else
    {
        while( sector->floorHeight * loader::QuarterSectorSize <= position.Y && sector->roomBelow != nullptr )
        {
            *room = sector->roomBelow;
            sector = (*room)->getSectorByAbsolutePosition( position );
            Expects( sector != nullptr );
        }
    }

    return sector;
}

gsl::not_null<const loader::Room*>
Level::findRoomForPosition(const core::TRCoordinates& position, gsl::not_null<const loader::Room*> room) const
{
    const loader::Sector* sector = nullptr;
    while( true )
    {
        sector = room->findFloorSectorWithClampedIndex(
                gsl::narrow_cast<int>( position.X - room->position.X ) / loader::SectorSize,
                gsl::narrow_cast<int>( position.Z - room->position.Z ) / loader::SectorSize );
        Expects( sector != nullptr );
        const auto portalTarget = engine::floordata::getPortalTarget( m_floorData, sector->floorDataIndex );
        if( !portalTarget )
        {
            break;
        }

        BOOST_ASSERT( *portalTarget != 0xff && *portalTarget < m_rooms.size() );
        room = &m_rooms[*portalTarget];
    }

    Expects( sector != nullptr );
    if( sector->floorHeight * loader::QuarterSectorSize > position.Y )
    {
        while( sector->ceilingHeight * loader::QuarterSectorSize > position.Y && sector->roomAbove != nullptr )
        {
            room = sector->roomAbove;
            sector = room->getSectorByAbsolutePosition( position );
            Expects( sector != nullptr );
        }
    }
    else
    {
        while( sector->floorHeight * loader::QuarterSectorSize <= position.Y && sector->roomBelow != nullptr )
        {
            room = sector->roomBelow;
            sector = room->getSectorByAbsolutePosition( position );
            Expects( sector != nullptr );
        }
    }

    return room;
}

engine::items::ItemNode* Level::getItemController(uint16_t id) const
{
    auto it = m_itemNodes.find( id );
    if( it == m_itemNodes.end() )
        return nullptr;

    return it->second.get();
}

void Level::drawBars(gameplay::Game* game, const std::shared_ptr<gameplay::gl::Image<gameplay::gl::RGBA8>>& image) const
{
    if( m_lara->isInWater() )
    {
        const int x0 = static_cast<const int>(game->getViewport().width - 110);

        for( int i = 7; i <= 13; ++i )
            image->line( x0 - 1, i, x0 + 101, i, m_palette->color[0].toTextureColor() );
        image->line( x0 - 2, 14, x0 + 102, 14, m_palette->color[17].toTextureColor() );
        image->line( x0 + 102, 6, x0 + 102, 14, m_palette->color[17].toTextureColor() );
        image->line( x0 + 102, 6, x0 + 102, 14, m_palette->color[19].toTextureColor() );
        image->line( x0 - 2, 6, x0 - 2, 14, m_palette->color[19].toTextureColor() );

        const int p = util::clamp( m_lara->getAir() * 100 / core::LaraAir, 0, 100 );
        if( p > 0 )
        {
            image->line( x0, 8, x0 + p, 8, m_palette->color[32].toTextureColor() );
            image->line( x0, 9, x0 + p, 9, m_palette->color[41].toTextureColor() );
            image->line( x0, 10, x0 + p, 10, m_palette->color[32].toTextureColor() );
            image->line( x0, 11, x0 + p, 11, m_palette->color[19].toTextureColor() );
            image->line( x0, 12, x0 + p, 12, m_palette->color[21].toTextureColor() );
        }
    }

    const int x0 = 8;
    for( int i = 7; i <= 13; ++i )
        image->line( x0 - 1, i, x0 + 101, i, m_palette->color[0].toTextureColor() );
    image->line( x0 - 2, 14, x0 + 102, 14, m_palette->color[17].toTextureColor() );
    image->line( x0 + 102, 6, x0 + 102, 14, m_palette->color[17].toTextureColor() );
    image->line( x0 + 102, 6, x0 + 102, 14, m_palette->color[19].toTextureColor() );
    image->line( x0 - 2, 6, x0 - 2, 14, m_palette->color[19].toTextureColor() );

    const int p = util::clamp( m_lara->m_state.health * 100 / core::LaraHealth, 0, 100 );
    if( p > 0 )
    {
        image->line( x0, 8, x0 + p, 8, m_palette->color[8].toTextureColor() );
        image->line( x0, 9, x0 + p, 9, m_palette->color[11].toTextureColor() );
        image->line( x0, 10, x0 + p, 10, m_palette->color[8].toTextureColor() );
        image->line( x0, 11, x0 + p, 11, m_palette->color[6].toTextureColor() );
        image->line( x0, 12, x0 + p, 12, m_palette->color[24].toTextureColor() );
    }
}

void Level::triggerCdTrack(uint16_t trackId, const engine::floordata::ActivationState& activationRequest,
                           engine::floordata::SequenceCondition triggerType)
{
    if( trackId < 1 || trackId >= 64 )
        return;

    if( trackId < 28 ) // music
    {
        triggerNormalCdTrack( trackId, activationRequest, triggerType );
        return;
    }

    if( trackId == 28 )
    {
        if( m_cdTrackActivationStates[trackId].isOneshot()
            && m_lara->getCurrentAnimState() == loader::LaraStateId::JumpUp )
            trackId = TrackWelcomeToMyHome;
        triggerNormalCdTrack( trackId, activationRequest, triggerType );
        return;
    }

    if( trackId == TrackWalkWontFallOff || trackId == 41 )
    {
        if( m_lara->getCurrentAnimState() == loader::LaraStateId::Hang )
            triggerNormalCdTrack( trackId, activationRequest, triggerType );
        return;
    }

    if( trackId >= TrackWelcomeToMyHome && trackId <= 40 )
    {
        triggerNormalCdTrack( trackId, activationRequest, triggerType );
        return;
    }

    if( trackId >= 42 && trackId <= 48 ) // gym tutorial voice lines
    {
        if( trackId == 42 && m_cdTrackActivationStates[42].isOneshot()
            && m_lara->getCurrentAnimState() == loader::LaraStateId::Hang )
            trackId = 43;
        triggerNormalCdTrack( trackId, activationRequest, triggerType );
        return;
    }

    if( trackId == TrackThenLetGo )
    {
        if( m_lara->getCurrentAnimState() == loader::LaraStateId::OnWaterStop )
            triggerNormalCdTrack( trackId, activationRequest, triggerType );
        return;
    }

    if( trackId == TrackLetsGoForASwim )
    {
        if( m_cdTrackActivationStates[TrackLetsGoForASwim].isOneshot() )
        {
            if( ++m_cdTrack50time == 120 )
            {
                //! @todo End level
                m_cdTrack50time = 0;
            }
            triggerNormalCdTrack( trackId, activationRequest, triggerType );
            return;
        }

        if( m_lara->getCurrentAnimState() == loader::LaraStateId::OnWaterExit )
            triggerNormalCdTrack( trackId, activationRequest, triggerType );
        return;
    }

    if( trackId >= 51 && trackId <= 63 ) // voice lines and SFX
    {
        triggerNormalCdTrack( trackId, activationRequest, triggerType );
    }
}

void Level::triggerNormalCdTrack(uint16_t trackId, const engine::floordata::ActivationState& activationRequest,
                                 engine::floordata::SequenceCondition triggerType)
{
    if( m_cdTrackActivationStates[trackId].isOneshot() )
        return;

    if( triggerType == engine::floordata::SequenceCondition::ItemActivated )
        m_cdTrackActivationStates[trackId] ^= activationRequest.getActivationSet();
    else if( triggerType == engine::floordata::SequenceCondition::LaraOnGroundInverted )
        m_cdTrackActivationStates[trackId] &= ~activationRequest.getActivationSet();
    else
        m_cdTrackActivationStates[trackId] |= activationRequest.getActivationSet();

    if( m_cdTrackActivationStates[trackId].isFullyActivated() )
    {
        if( activationRequest.isOneshot() )
            m_cdTrackActivationStates[trackId].setOneshot( true );

        if( m_activeCDTrack != trackId )
            playCdTrack( trackId );
    }
    else
    {
        stopCdTrack( trackId );
    }
}

void Level::playCdTrack(uint16_t trackId)
{
    if( trackId == TrackActionMusic )
    {
        m_lara->playSoundEffect( 173 );
        return;
    }

    if( trackId > 2 && trackId < 22 )
        return;

    BOOST_LOG_TRIVIAL( debug ) << "Stopping track #" << m_activeCDTrack;

    if( m_activeCDTrack >= 26 && m_activeCDTrack <= 56 )
    {
        stopSoundEffect( static_cast<uint16_t>(m_activeCDTrack + 148) );
    }
    else if( m_activeCDTrack > 0 )
    {
        m_audioDev.removeStream( m_cdStream );
        m_cdStream.reset();
    }
    m_activeCDTrack = 0;

    BOOST_LOG_TRIVIAL( debug ) << "Playing track #" << trackId;

    if( trackId >= 26 && trackId <= 56 )
    {
        m_lara->playSoundEffect( trackId + 148 );
        m_activeCDTrack = trackId;
        return;
    }

    if( trackId == 2 )
    {
        trackId = 2;
    }
    else if( trackId >= 22 && trackId <= 25 ) // non-ambient (cinematic) music
    {
        trackId -= 15;
    }
    else
    {
        if( trackId <= 56 )
        {
            m_activeCDTrack = trackId;
            return;
        }
        trackId -= 54;
    }

    playStream( trackId );
    m_activeCDTrack = trackId;
}

void Level::stopCdTrack(uint16_t trackId)
{
    if( m_activeCDTrack == 0 )
        return;

    if( m_activeCDTrack < 26 || m_activeCDTrack > 56 )
    {
        m_audioDev.removeStream( m_cdStream );
        m_cdStream.reset();
    }
    else
    {
        stopSoundEffect( static_cast<uint16_t>(trackId + 148) );
    }

    m_activeCDTrack = 0;
}

void Level::playStream(uint16_t trackId)
{
    static constexpr size_t DefaultBufferSize = 16384;

    m_audioDev.removeStream( m_cdStream );
    m_cdStream.reset();

    if( boost::filesystem::is_regular_file( "data/tr1/audio/CDAUDIO.WAD" ) )
        m_cdStream = std::make_unique<audio::Stream>(
                std::make_unique<audio::WadStreamSource>( "data/tr1/audio/CDAUDIO.WAD", trackId ), DefaultBufferSize );
    else
        m_cdStream = std::make_unique<audio::Stream>( std::make_unique<audio::SndfileStreamSource>(
                (boost::format( "data/tr1/audio/%03d.ogg" ) % trackId).str() ), DefaultBufferSize );

    m_audioDev.registerStream( m_cdStream );
}

void Level::useAlternativeLaraAppearance()
{
    const auto& base = *m_animatedModels[0];
    BOOST_ASSERT( base.nmeshes == m_lara->getNode()->getChildCount() );

    const auto& alternate = *m_animatedModels[5];
    BOOST_ASSERT( alternate.nmeshes == m_lara->getNode()->getChildCount() );

    for( size_t i = 0; i < m_lara->getNode()->getChildCount(); ++i )
        m_lara->getNode()->getChild( i )->setDrawable( m_models[m_meshIndices[alternate.frame_number + i]] );

    // Don't replace the head.
    m_lara->getNode()->getChild( 14 )->setDrawable( m_models[m_meshIndices[base.frame_number + 14]] );
}

void Level::postProcessDataStructures()
{
    BOOST_LOG_TRIVIAL(info) << "Post-processing data structures";

    for(loader::Room& room : m_rooms)
    {
        for(loader::Sector& sector : room.sectors)
        {
            if(sector.boxIndex >= 0)
            {
                Expects(static_cast<size_t>(sector.boxIndex) < m_boxes.size());
                sector.box = &m_boxes[sector.boxIndex];
            }

            if(sector.roomIndexBelow != 0xff)
            {
                Expects(sector.roomIndexBelow < m_rooms.size());
                sector.roomBelow = &m_rooms[sector.roomIndexBelow];
            }

            if(sector.roomIndexAbove != 0xff)
            {
                Expects(sector.roomIndexAbove < m_rooms.size());
                sector.roomAbove = &m_rooms[sector.roomIndexAbove];
            }
        }
    }

    Expects(m_baseZones.flyZone.size() == m_boxes.size());
    Expects(m_baseZones.groundZone1.size() == m_boxes.size());
    Expects(m_baseZones.groundZone2.size() == m_boxes.size());
    Expects(m_alternateZones.flyZone.size() == m_boxes.size());
    Expects(m_alternateZones.groundZone1.size() == m_boxes.size());
    Expects(m_alternateZones.groundZone2.size() == m_boxes.size());
    for(size_t i=0; i<m_boxes.size(); ++i)
    {
        m_boxes[i].zoneFly = m_baseZones.flyZone[i];
        m_boxes[i].zoneGround1 = m_baseZones.groundZone1[i];
        m_boxes[i].zoneGround2 = m_baseZones.groundZone2[i];
        m_boxes[i].zoneFlySwapped = m_alternateZones.flyZone[i];
        m_boxes[i].zoneGround1Swapped = m_alternateZones.groundZone1[i];
        m_boxes[i].zoneGround2Swapped = m_alternateZones.groundZone2[i];
    }

    for(const auto& model : m_animatedModels | boost::adaptors::map_values)
    {
        Expects(model->pose_data_offset % 2 == 0);

        const auto idx = model->pose_data_offset / 2;
        if(idx >= m_poseData.size())
        {
            BOOST_LOG_TRIVIAL(warning) << "Pose data index " << idx << " out of range 0.." << m_poseData.size()-1;
            continue;
        }
        model->frame_base = reinterpret_cast<const loader::AnimFrame*>(&m_poseData[idx]);
    }

    for(auto& anim : m_animations)
    {
        Expects(anim.poseDataOffset % 2 == 0);

        const auto idx = anim.poseDataOffset / 2;
        if(idx >= m_poseData.size())
        {
            BOOST_LOG_TRIVIAL(warning) << "Pose data index " << idx << " out of range 0.." << m_poseData.size()-1;
            continue;
        }
        anim.poseData = reinterpret_cast<const loader::AnimFrame*>(&m_poseData[idx]);
    }
}