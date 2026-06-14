#pragma once
#include "../render.hpp"
#include "../texture.hpp"

#if defined(_WIN32)
	#include <Windows.h>
	#include <GL/gl.h>
#elif defined(__APPLE__)
	#include <OpenGL/gl.h>
	#include <dlfcn.h>
#else
	#include <GL/gl.h>
	#include <GL/glx.h>
#endif

#ifndef APIENTRY
	#define APIENTRY
#endif

// GL types not in base gl.h on Windows
#ifndef GL_FRAGMENT_SHADER
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
#endif

// GL constants (guarded for platforms that already define them)
#ifndef GL_FRAGMENT_SHADER
	#define GL_FRAGMENT_SHADER                0x8B30
#endif
#ifndef GL_VERTEX_SHADER
	#define GL_VERTEX_SHADER                  0x8B31
#endif
#ifndef GL_COMPILE_STATUS
	#define GL_COMPILE_STATUS                 0x8B81
#endif
#ifndef GL_LINK_STATUS
	#define GL_LINK_STATUS                    0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
	#define GL_INFO_LOG_LENGTH                0x8B84
#endif
#ifndef GL_ARRAY_BUFFER
	#define GL_ARRAY_BUFFER                   0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
	#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#endif
#ifndef GL_UNIFORM_BUFFER
	#define GL_UNIFORM_BUFFER                 0x8A11
#endif
#ifndef GL_DYNAMIC_DRAW
	#define GL_DYNAMIC_DRAW                   0x88E8
#endif
#ifndef GL_TEXTURE0
	#define GL_TEXTURE0                       0x84C0
#endif
#ifndef GL_CLAMP_TO_EDGE
	#define GL_CLAMP_TO_EDGE                  0x812F
#endif
#ifndef GL_STREAM_DRAW
	#define GL_STREAM_DRAW                    0x88E0
#endif
#ifndef GL_FRAMEBUFFER
	#define GL_FRAMEBUFFER                    0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
	#define GL_COLOR_ATTACHMENT0              0x8CE0
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
	#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#endif

// GL function pointer types
typedef GLuint (APIENTRY* PFN_glCreateShader)(GLenum type);
typedef void   (APIENTRY* PFN_glShaderSource)(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
typedef void   (APIENTRY* PFN_glCompileShader)(GLuint shader);
typedef GLuint (APIENTRY* PFN_glCreateProgram)();
typedef void   (APIENTRY* PFN_glAttachShader)(GLuint program, GLuint shader);
typedef void   (APIENTRY* PFN_glLinkProgram)(GLuint program);
typedef void   (APIENTRY* PFN_glUseProgram)(GLuint program);
typedef GLint  (APIENTRY* PFN_glGetUniformLocation)(GLuint program, const GLchar* name);
typedef void   (APIENTRY* PFN_glUniform1i)(GLint location, GLint v0);
typedef void   (APIENTRY* PFN_glUniform1f)(GLint location, GLfloat v0);
typedef void   (APIENTRY* PFN_glUniform2f)(GLint location, GLfloat v0, GLfloat v1);
typedef void   (APIENTRY* PFN_glUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void   (APIENTRY* PFN_glBindAttribLocation)(GLuint program, GLuint index, const GLchar* name);
typedef void   (APIENTRY* PFN_glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
typedef void   (APIENTRY* PFN_glGetProgramiv)(GLuint program, GLenum pname, GLint* params);
typedef void   (APIENTRY* PFN_glGetShaderInfoLog)(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
typedef void   (APIENTRY* PFN_glGetProgramInfoLog)(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
typedef void   (APIENTRY* PFN_glDeleteShader)(GLuint shader);
typedef void   (APIENTRY* PFN_glDeleteProgram)(GLuint program);
typedef void   (APIENTRY* PFN_glDetachShader)(GLuint program, GLuint shader);
typedef void   (APIENTRY* PFN_glGenBuffers)(GLsizei n, GLuint* buffers);
typedef void   (APIENTRY* PFN_glDeleteBuffers)(GLsizei n, const GLuint* buffers);
typedef void   (APIENTRY* PFN_glBindBuffer)(GLenum target, GLuint buffer);
typedef void   (APIENTRY* PFN_glBufferData)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
typedef void   (APIENTRY* PFN_glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);
typedef void   (APIENTRY* PFN_glEnableVertexAttribArray)(GLuint index);
typedef void   (APIENTRY* PFN_glDisableVertexAttribArray)(GLuint index);
typedef void   (APIENTRY* PFN_glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
typedef void   (APIENTRY* PFN_glBlendFuncSeparate)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
typedef void   (APIENTRY* PFN_glActiveTexture)(GLenum texture);
// GL3-only (optional, null on GL2 contexts)
typedef void   (APIENTRY* PFN_glBindBufferBase)(GLenum target, GLuint index, GLuint buffer);
typedef GLuint (APIENTRY* PFN_glGetUniformBlockIndex)(GLuint program, const GLchar* uniformBlockName);
typedef void   (APIENTRY* PFN_glUniformBlockBinding)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
typedef void   (APIENTRY* PFN_glGenVertexArrays)(GLsizei n, GLuint* arrays);
typedef void   (APIENTRY* PFN_glBindVertexArray)(GLuint array);
typedef void   (APIENTRY* PFN_glDeleteVertexArrays)(GLsizei n, const GLuint* arrays);
// FBO (core in GL3, available as extension in GL2)
typedef void   (APIENTRY* PFN_glGenFramebuffers)(GLsizei n, GLuint* framebuffers);
typedef void   (APIENTRY* PFN_glDeleteFramebuffers)(GLsizei n, const GLuint* framebuffers);
typedef void   (APIENTRY* PFN_glBindFramebuffer)(GLenum target, GLuint framebuffer);
typedef void   (APIENTRY* PFN_glFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum (APIENTRY* PFN_glCheckFramebufferStatus)(GLenum target);

namespace rv
{
	// single set of GL function pointers, loaded once
	namespace gl
	{
		extern PFN_glCreateShader CreateShader;
		extern PFN_glShaderSource ShaderSource;
		extern PFN_glCompileShader CompileShader;
		extern PFN_glCreateProgram CreateProgram;
		extern PFN_glAttachShader AttachShader;
		extern PFN_glLinkProgram LinkProgram;
		extern PFN_glUseProgram UseProgram;
		extern PFN_glGetUniformLocation GetUniformLocation;
		extern PFN_glUniform1i Uniform1i;
		extern PFN_glUniform1f Uniform1f;
		extern PFN_glUniform2f Uniform2f;
		extern PFN_glUniform4f Uniform4f;
		extern PFN_glBindAttribLocation BindAttribLocation;
		extern PFN_glGetShaderiv GetShaderiv;
		extern PFN_glGetProgramiv GetProgramiv;
		extern PFN_glGetShaderInfoLog GetShaderInfoLog;
		extern PFN_glGetProgramInfoLog GetProgramInfoLog;
		extern PFN_glDeleteShader DeleteShader;
		extern PFN_glDeleteProgram DeleteProgram;
		extern PFN_glDetachShader DetachShader;
		extern PFN_glGenBuffers GenBuffers;
		extern PFN_glDeleteBuffers DeleteBuffers;
		extern PFN_glBindBuffer BindBuffer;
		extern PFN_glBufferData BufferData;
		extern PFN_glBufferSubData BufferSubData;
		extern PFN_glEnableVertexAttribArray EnableVertexAttribArray;
		extern PFN_glDisableVertexAttribArray DisableVertexAttribArray;
		extern PFN_glVertexAttribPointer VertexAttribPointer;
		extern PFN_glBlendFuncSeparate BlendFuncSeparate;
		extern PFN_glActiveTexture ActiveTexture;
		// GL3-only (null on GL2)
		extern PFN_glBindBufferBase BindBufferBase;
		extern PFN_glGetUniformBlockIndex GetUniformBlockIndex;
		extern PFN_glUniformBlockBinding UniformBlockBinding;
		extern PFN_glGenVertexArrays GenVertexArrays;
		extern PFN_glBindVertexArray BindVertexArray;
		extern PFN_glDeleteVertexArrays DeleteVertexArrays;
		// FBO (optional, needed for backdrop blur)
		extern PFN_glGenFramebuffers GenFramebuffers;
		extern PFN_glDeleteFramebuffers DeleteFramebuffers;
		extern PFN_glBindFramebuffer BindFramebuffer;
		extern PFN_glFramebufferTexture2D FramebufferTexture2D;
		extern PFN_glCheckFramebufferStatus CheckFramebufferStatus;
	}

	class ogl_texture : public texture
	{
	public:
		explicit ogl_texture(renderer* const renderer, GLuint id, const cstd::uint32_t width = 0, const cstd::uint32_t height = 0) noexcept
				:	texture(renderer, width, height),
					id_(id) { }

		ogl_texture(const ogl_texture&) = delete;
		ogl_texture& operator=(const ogl_texture&) = delete;

		ogl_texture(ogl_texture&& other) noexcept
				:	texture(other.renderer_, other.width_, other.height_),
					id_(other.id_)
		{
			other.id_ = 0;
		}

		~ogl_texture()
		{
			if (id_)
			{
				glDeleteTextures(1, &id_);
			}
		}

		[[nodiscard]] GLuint id() const noexcept
		{
			return id_;
		}

	protected:
		GLuint id_ = 0;
	};

	class ogl_renderer : public renderer
	{
	public:
		~ogl_renderer();

		void end_frame() noexcept override;
		shared_ptr_t<texture> create_texture(span_t<const cstd::uint8_t> buffer, cstd::uint32_t width, cstd::uint32_t height) override;
		shared_ptr_t<texture> create_texture_from_srv(void* srv) override;

	protected:
		ogl_renderer() noexcept { rt_uv_flipped_ = true; }

		bool load_gl_functions(bool require_ubo) noexcept;
		GLuint compile_shader(GLenum type, const char* source) noexcept;
		GLuint create_program(const char* vertex_src, const char* fragment_src, bool bind_attribs) noexcept;

		void begin_frame_backend(vector_2d<float> display_size) noexcept override;
		void flush_pending_vertices() noexcept override;

		shared_ptr_t<texture> create_render_target(cstd::uint32_t width, cstd::uint32_t height) override;
		void capture_backbuffer_region(const shared_ptr_t<texture>& dst, cstd::uint32_t x, cstd::uint32_t y, cstd::uint32_t w, cstd::uint32_t h) override;
		void set_render_target(const shared_ptr_t<texture>& target) override;
		void reset_render_target() override;

		static constexpr cstd::size_t shader_count_ = 6;
		GLuint programs_[shader_count_] = { };

		struct program_uniforms
		{
			GLint display_size = -1;
			GLint texture = -1;
			GLint clip_min = -1;
			GLint clip_max = -1;
			GLint clip_radii = -1;
			GLint clip_enabled = -1;
		};

		program_uniforms uniforms_[shader_count_] = { };

		// non-zero when using UBO clip path (GL3)
		GLuint clip_ubo_ = 0;

		GLuint vbo_ = 0;
		GLuint ebo_ = 0;
		GLuint vao_ = 0;

		vector_t<cstd::uint32_t> adjusted_indices_;

		GLuint blur_fbo_ = 0;
		GLint saved_viewport_[4] = {};
	};

	class ogl2_renderer : public ogl_renderer
	{
	public:
		ogl2_renderer() noexcept = default;

	protected:
		bool init_backend() noexcept override;
	};

	class ogl3_renderer : public ogl_renderer
	{
	public:
		ogl3_renderer() noexcept = default;

	protected:
		bool init_backend() noexcept override;
	};
}
