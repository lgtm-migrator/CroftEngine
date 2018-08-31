#pragma once

#include "Base.h"
#include "gl/util.h"

#include <boost/optional.hpp>

#include <map>
#include <memory>
#include <vector>

namespace gameplay
{
class RenderState final
{
public:
    RenderState(const RenderState&) = default;

    RenderState& operator=(const RenderState&) = delete;

    explicit RenderState() = default;

    ~RenderState() = default;

    void bindState(bool force = false) const;

    void setBlend(bool enabled);

    void setBlendSrc(GLenum blend);

    void setBlendDst(GLenum blend);

    void setCullFace(bool enabled);

    void setCullFaceSide(GLenum side);

    void setFrontFace(GLenum winding);

    void setDepthTest(bool enabled);

    void setDepthWrite(bool enabled);

    void setDepthFunction(GLenum func);

    static void initDefaults();

    static void enableDepthWrite();

    void merge(const RenderState& other);

private:
    template<typename T, const T DefaultValue>
    struct DefaultedOptional
    {
        static const constexpr T Default = DefaultValue;
        boost::optional<T> value{};

        T get() const
        {
            return value.get_value_or( Default );
        }

        void reset()
        {
            value.reset();
        }

        void setDefault()
        {
            value = Default;
        }

        bool isInitialized() const
        {
            return value.is_initialized();
        }

        bool operator!=(const DefaultedOptional<T, DefaultValue>& rhs) const
        {
            return value != rhs.value;
        }

        DefaultedOptional<T, DefaultValue>& operator=(T rhs)
        {
            value = rhs;
            return *this;
        }

        void merge(const DefaultedOptional<T, DefaultValue>& other)
        {
            if( other.isInitialized() )
                *this = other;
        }
    };


    // States
    DefaultedOptional<bool, false> m_cullFaceEnabled;

    DefaultedOptional<bool, true> m_depthTestEnabled;

    DefaultedOptional<bool, true> m_depthWriteEnabled;

    DefaultedOptional<GLenum, GL_LESS> m_depthFunction;

    DefaultedOptional<bool, true> m_blendEnabled;

    DefaultedOptional<GLenum, GL_SRC_ALPHA> m_blendSrc;

    DefaultedOptional<GLenum, GL_ONE_MINUS_SRC_ALPHA> m_blendDst;

    DefaultedOptional<GLenum, GL_BACK> m_cullFaceSide;

    DefaultedOptional<GLenum, GL_CW> m_frontFace;

    static RenderState s_currentState;
};
}
