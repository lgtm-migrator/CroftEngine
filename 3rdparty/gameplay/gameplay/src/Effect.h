#pragma once

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Texture.h"


namespace gameplay
{
    class Uniform;


    /**
     * Defines an effect which can be applied during rendering.
     *
     * An effect essentially wraps an OpenGL program object, which includes the
     * vertex and fragment shader.
     *
     * In the future, this class may be extended to support additional logic that
     * typical effect systems support, such as GPU render state management,
     * techniques and passes.
     */
    class Effect : std::enable_shared_from_this<Effect>
    {
    public:
        explicit Effect();
        ~Effect();

        /**
         * Creates an effect using the specified vertex and fragment shader.
         *
         * @param vshPath The path to the vertex shader file.
         * @param fshPath The path to the fragment shader file.
         * @param defines A new-line delimited list of preprocessor defines. May be nullptr.
         *
         * @return The created effect.
         */
        static std::shared_ptr<Effect> createFromFile(const char* vshPath, const char* fshPath, const char* defines = nullptr);

        /**
         * Creates an effect from the given vertex and fragment shader source code.
         *
         * @param vshSource The vertex shader source code.
         * @param fshSource The fragment shader source code.
         * @param defines A new-line delimited list of preprocessor defines. May be nullptr.
         *
         * @return The created effect.
         */
        static std::shared_ptr<Effect> createFromSource(const char* vshSource, const char* fshSource, const char* defines = nullptr);

        /**
         * Returns the unique string identifier for the effect, which is a concatenation of
         * the shader paths it was loaded from.
         */
        const std::string& getId() const;

        /**
         * Returns the vertex attribute handle for the vertex attribute with the specified name.
         *
         * @param name The name of the vertex attribute to return.
         *
         * @return The vertex attribute, or -1 if no such vertex attribute exists.
         */
        VertexAttribute getVertexAttribute(const char* name) const;

        /**
         * Returns the uniform handle for the uniform with the specified name.
         *
         * @param name The name of the uniform to return.
         *
         * @return The uniform, or nullptr if no such uniform exists.
         */
        Uniform* getUniform(const char* name) const;

        /**
         * Returns the specified active uniform.
         *
         * @param index The index of the uniform to return.
         *
         * @return The uniform, or nullptr if index is invalid.
         */
        Uniform* getUniform(unsigned int index) const;

        /**
         * Returns the number of active uniforms in this effect.
         *
         * @return The number of active uniforms.
         */
        size_t getUniformCount() const;

        /**
         * Sets a float uniform value.
         *
         * @param uniform The uniform to set.
         * @param value The float value to set.
         */
        void setValue(Uniform* uniform, float value);

        /**
         * Sets a float array uniform value.
         *
         * @param uniform The uniform to set.
         * @param values The array to set.
         * @param count The number of elements in the array.
         */
        void setValue(Uniform* uniform, const float* values, unsigned int count = 1);

        /**
         * Sets an integer uniform value.
         *
         * @param uniform The uniform to set.
         * @param value The value to set.
         */
        void setValue(Uniform* uniform, int value);

        /**
         * Sets an integer array uniform value.
         *
         * @param uniform The uniform to set.
         * @param values The array to set.
         * @param count The number of elements in the array.
         */
        void setValue(Uniform* uniform, const int* values, unsigned int count = 1);

        /**
         * Sets a matrix uniform value.
         *
         * @param uniform The uniform to set.
         * @param value The value to set.
         */
        void setValue(Uniform* uniform, const Matrix& value);

        /**
         * Sets a matrix array uniform value.
         *
         * @param uniform The uniform to set.
         * @param values The array to set.
         * @param count The number of elements in the array.
         */
        void setValue(Uniform* uniform, const Matrix* values, unsigned int count = 1);

        /**
         * Sets a vector uniform value.
         *
         * @param uniform The uniform to set.
         * @param value The value to set.
         */
        void setValue(Uniform* uniform, const Vector2& value);

        /**
         * Sets a vector array uniform value.
         *
         * @param uniform The uniform to set.
         * @param values The array to set.
         * @param count The number of elements in the array.
         */
        void setValue(Uniform* uniform, const Vector2* values, unsigned int count = 1);

        /**
         * Sets a vector uniform value.
         *
         * @param uniform The uniform to set.
         * @param value The value to set.
         */
        void setValue(Uniform* uniform, const Vector3& value);

        /**
         * Sets a vector array uniform value.
         *
         * @param uniform The uniform to set.
         * @param values The array to set.
         * @param count The number of elements in the array.
         */
        void setValue(Uniform* uniform, const Vector3* values, unsigned int count = 1);

        /**
         * Sets a vector uniform value.
         *
         * @param uniform The uniform to set.
         * @param value The value to set.
         */
        void setValue(Uniform* uniform, const Vector4& value);

        /**
         * Sets a vector array uniform value.
         *
         * @param uniform The uniform to set.
         * @param values The array to set.
         * @param count The number of elements in the array.
         */
        void setValue(Uniform* uniform, const Vector4* values, unsigned int count = 1);

        /**
         * Sets a sampler uniform value.
         *
         * @param uniform The uniform to set.
         * @param sampler The sampler to set.
         */
        void setValue(Uniform* uniform, const std::shared_ptr<Texture::Sampler>& sampler);

        /**
         * Sets a sampler array uniform value.
         *
         * @param uniform The uniform to set.
         * @param values The sampler array to set.
         *
         * @script{ignore}
         */
        void setValue(Uniform* uniform, const std::vector<std::shared_ptr<Texture::Sampler>>& values);

        /**
         * Binds this effect to make it the currently active effect for the rendering system.
         */
        void bind();

        /**
         * Returns the currently bound effect for the rendering system.
         *
         * @return The currently bound effect, or nullptr if no effect is currently bound.
         */
        static Effect* getCurrentEffect();

    private:

        Effect& operator=(const Effect&) = delete;

        static std::shared_ptr<Effect> createFromSource(const char* vshPath, const char* vshSource, const char* fshPath, const char* fshSource, const char* defines = nullptr);

        GLuint _program;
        std::string _id;
        std::map<std::string, VertexAttribute> _vertexAttributes;
        mutable std::map<std::string, Uniform*> _uniforms;
        static Uniform _emptyUniform;
    };


    /**
     * Represents a uniform variable within an effect.
     */
    class Uniform
    {
        friend class Effect;

    public:

        /**
         * Returns the name of this uniform.
         *
         * @return The name of the uniform.
         */
        const char* getName() const;

        /**
         * Returns the OpenGL uniform type.
         *
         * @return The OpenGL uniform type.
         */
        GLenum getType() const;

        /**
         * Returns the effect for this uniform.
         *
         * @return The uniform's effect.
         */
        const std::shared_ptr<Effect>& getEffect() const;

    private:

        /**
         * Constructor.
         */
        Uniform();

        /**
         * Copy constructor.
         */
        Uniform(const Uniform& copy);

        /**
         * Destructor.
         */
        ~Uniform();

        /**
         * Hidden copy assignment operator.
         */
        Uniform& operator=(const Uniform&);

        std::string _name;
        GLint _location;
        GLenum _type;
        unsigned int _index;
        std::shared_ptr<Effect> _effect;
    };
}
