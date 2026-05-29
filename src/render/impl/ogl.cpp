#include "ogl.hpp"
#include "../ogl_shaders.hpp"
#include "../../log/log.hpp"

namespace
{
	struct alignas(16) clip_ubo_data
	{
		float clip_min[2];
		float clip_max[2];
		float clip_radii[4];
		float clip_enabled;
		float padding[3];
	};

	constexpr cstd::uint32_t clip_binding_point = 0;

	void* load_proc(const char* name)
	{
#if defined(_WIN32)
		void* p = reinterpret_cast<void*>(wglGetProcAddress(name));
		if (!p || p == reinterpret_cast<void*>(1) || p == reinterpret_cast<void*>(2) ||
			p == reinterpret_cast<void*>(3) || p == reinterpret_cast<void*>(-1))
		{
			return nullptr;
		}
		return p;
#elif defined(__APPLE__)
		return dlsym(RTLD_DEFAULT, name);
#else
		return reinterpret_cast<void*>(glXGetProcAddressARB(reinterpret_cast<const GLubyte*>(name)));
#endif
	}
}

// GL function pointer definitions
namespace rv::gl
{
	PFN_glCreateShader CreateShader = nullptr;
	PFN_glShaderSource ShaderSource = nullptr;
	PFN_glCompileShader CompileShader = nullptr;
	PFN_glCreateProgram CreateProgram = nullptr;
	PFN_glAttachShader AttachShader = nullptr;
	PFN_glLinkProgram LinkProgram = nullptr;
	PFN_glUseProgram UseProgram = nullptr;
	PFN_glGetUniformLocation GetUniformLocation = nullptr;
	PFN_glUniform1i Uniform1i = nullptr;
	PFN_glUniform1f Uniform1f = nullptr;
	PFN_glUniform2f Uniform2f = nullptr;
	PFN_glUniform4f Uniform4f = nullptr;
	PFN_glBindAttribLocation BindAttribLocation = nullptr;
	PFN_glGetShaderiv GetShaderiv = nullptr;
	PFN_glGetProgramiv GetProgramiv = nullptr;
	PFN_glGetShaderInfoLog GetShaderInfoLog = nullptr;
	PFN_glGetProgramInfoLog GetProgramInfoLog = nullptr;
	PFN_glDeleteShader DeleteShader = nullptr;
	PFN_glDeleteProgram DeleteProgram = nullptr;
	PFN_glDetachShader DetachShader = nullptr;
	PFN_glGenBuffers GenBuffers = nullptr;
	PFN_glDeleteBuffers DeleteBuffers = nullptr;
	PFN_glBindBuffer BindBuffer = nullptr;
	PFN_glBufferData BufferData = nullptr;
	PFN_glBufferSubData BufferSubData = nullptr;
	PFN_glEnableVertexAttribArray EnableVertexAttribArray = nullptr;
	PFN_glDisableVertexAttribArray DisableVertexAttribArray = nullptr;
	PFN_glVertexAttribPointer VertexAttribPointer = nullptr;
	PFN_glBlendFuncSeparate BlendFuncSeparate = nullptr;
	PFN_glActiveTexture ActiveTexture = nullptr;
	PFN_glBindBufferBase BindBufferBase = nullptr;
	PFN_glGetUniformBlockIndex GetUniformBlockIndex = nullptr;
	PFN_glUniformBlockBinding UniformBlockBinding = nullptr;
}

bool rv::ogl_renderer::load_gl_functions(const bool require_ubo) noexcept
{
#define RV_GL(name) \
	gl::name = reinterpret_cast<PFN_gl##name>(load_proc("gl" #name)); \
	if (!gl::name) return false

	RV_GL(CreateShader);       RV_GL(ShaderSource);         RV_GL(CompileShader);
	RV_GL(CreateProgram);      RV_GL(AttachShader);         RV_GL(LinkProgram);
	RV_GL(UseProgram);         RV_GL(GetUniformLocation);
	RV_GL(Uniform1i);         RV_GL(Uniform1f);            RV_GL(Uniform2f);            RV_GL(Uniform4f);
	RV_GL(BindAttribLocation); RV_GL(GetShaderiv);          RV_GL(GetProgramiv);
	RV_GL(GetShaderInfoLog);   RV_GL(GetProgramInfoLog);
	RV_GL(DeleteShader);       RV_GL(DeleteProgram);        RV_GL(DetachShader);
	RV_GL(GenBuffers);         RV_GL(DeleteBuffers);        RV_GL(BindBuffer);
	RV_GL(BufferData);         RV_GL(BufferSubData);
	RV_GL(EnableVertexAttribArray); RV_GL(DisableVertexAttribArray); RV_GL(VertexAttribPointer);
	RV_GL(BlendFuncSeparate);  RV_GL(ActiveTexture);

#undef RV_GL

	// GL3-only (optional — null is fine for GL2)
#define RV_GL_OPT(name) gl::name = reinterpret_cast<PFN_gl##name>(load_proc("gl" #name))

	RV_GL_OPT(BindBufferBase); RV_GL_OPT(GetUniformBlockIndex); RV_GL_OPT(UniformBlockBinding);

#undef RV_GL_OPT

	if (require_ubo && (!gl::BindBufferBase || !gl::GetUniformBlockIndex || !gl::UniformBlockBinding))
	{
		return false;
	}

	return true;
}

GLuint rv::ogl_renderer::compile_shader(const GLenum type, const char* source) noexcept
{
	const GLuint shader = gl::CreateShader(type);
	gl::ShaderSource(shader, 1, &source, nullptr);
	gl::CompileShader(shader);

	GLint status = 0;
	gl::GetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (!status)
	{
		GLint log_length = 0;
		gl::GetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

		if (log_length > 0)
		{
			vector_t<char> log(static_cast<cstd::size_t>(log_length));
			gl::GetShaderInfoLog(shader, log_length, nullptr, log.data());
			LOG_ERR("ogl shader compile error: {}", log.data());
		}

		gl::DeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint rv::ogl_renderer::create_program(const char* vertex_src, const char* fragment_src, const bool bind_attribs) noexcept
{
	const GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_src);
	if (!vs) return 0;

	const GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_src);
	if (!fs)
	{
		gl::DeleteShader(vs);
		return 0;
	}

	const GLuint program = gl::CreateProgram();

	// GL2 needs manual attrib locations (GLSL 120 has no layout(location=N))
	if (bind_attribs)
	{
		gl::BindAttribLocation(program, 0, "a_position");
		gl::BindAttribLocation(program, 1, "a_color");
		gl::BindAttribLocation(program, 2, "a_uv");
		gl::BindAttribLocation(program, 3, "a_custom_data");
		gl::BindAttribLocation(program, 4, "a_custom_data2");
	}

	gl::AttachShader(program, vs);
	gl::AttachShader(program, fs);
	gl::LinkProgram(program);

	GLint status = 0;
	gl::GetProgramiv(program, GL_LINK_STATUS, &status);

	if (!status)
	{
		GLint log_length = 0;
		gl::GetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

		if (log_length > 0)
		{
			vector_t<char> log(static_cast<cstd::size_t>(log_length));
			gl::GetProgramInfoLog(program, log_length, nullptr, log.data());
			LOG_ERR("ogl program link error: {}", log.data());
		}

		gl::DeleteProgram(program);
		gl::DeleteShader(vs);
		gl::DeleteShader(fs);
		return 0;
	}

	gl::DetachShader(program, vs);
	gl::DetachShader(program, fs);
	gl::DeleteShader(vs);
	gl::DeleteShader(fs);

	return program;
}

rv::ogl_renderer::~ogl_renderer()
{
	for (auto& program : programs_)
	{
		if (program && gl::DeleteProgram)
		{
			gl::DeleteProgram(program);
			program = 0;
		}
	}

	if (clip_ubo_ && gl::DeleteBuffers)
	{
		gl::DeleteBuffers(1, &clip_ubo_);
	}
}

bool rv::ogl2_renderer::init_backend() noexcept
{
	if (!load_gl_functions(false))
	{
		LOG_ERR("failed to load GL functions");
		return false;
	}

	const auto& shaders = ogl_shaders::gl2::set;
	const char* fragment_sources[shader_count_] =
	{
		shaders.default_fragment, shaders.shadow_fragment,
		shaders.rect_fragment, shaders.image_fragment, shaders.text_shadow_fragment
	};

	for (cstd::size_t i = 0; i < shader_count_; ++i)
	{
		programs_[i] = create_program(shaders.vertex, fragment_sources[i], true);
		if (!programs_[i])
		{
			LOG_ERR("failed to create ogl2 shader program {}", i);
			return false;
		}

		uniforms_[i].display_size = gl::GetUniformLocation(programs_[i], "u_display_size");
		uniforms_[i].texture = gl::GetUniformLocation(programs_[i], "u_texture");
		uniforms_[i].clip_min = gl::GetUniformLocation(programs_[i], "u_clip_min");
		uniforms_[i].clip_max = gl::GetUniformLocation(programs_[i], "u_clip_max");
		uniforms_[i].clip_radii = gl::GetUniformLocation(programs_[i], "u_clip_radii");
		uniforms_[i].clip_enabled = gl::GetUniformLocation(programs_[i], "u_clip_enabled");

		gl::UseProgram(programs_[i]);
		gl::Uniform1i(uniforms_[i].texture, 0);
	}

	gl::UseProgram(0);

	glEnable(GL_BLEND);
	gl::BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	return true;
}

bool rv::ogl3_renderer::init_backend() noexcept
{
	if (!load_gl_functions(true))
	{
		LOG_ERR("failed to load GL3 functions");
		return false;
	}

	const auto& shaders = ogl_shaders::gl3::set;
	const char* fragment_sources[shader_count_] =
	{
		shaders.default_fragment, shaders.shadow_fragment,
		shaders.rect_fragment, shaders.image_fragment, shaders.text_shadow_fragment
	};

	for (cstd::size_t i = 0; i < shader_count_; ++i)
	{
		programs_[i] = create_program(shaders.vertex, fragment_sources[i], false);
		if (!programs_[i])
		{
			LOG_ERR("failed to create ogl3 shader program {}", i);
			return false;
		}

		uniforms_[i].display_size = gl::GetUniformLocation(programs_[i], "u_display_size");
		uniforms_[i].texture = gl::GetUniformLocation(programs_[i], "u_texture");

		gl::UseProgram(programs_[i]);
		gl::Uniform1i(uniforms_[i].texture, 0);

		const GLuint block_idx = gl::GetUniformBlockIndex(programs_[i], "ClipBuffer");
		if (block_idx != 0xFFFFFFFF)
		{
			gl::UniformBlockBinding(programs_[i], block_idx, clip_binding_point);
		}
	}

	gl::UseProgram(0);

	gl::GenBuffers(1, &clip_ubo_);
	gl::BindBuffer(GL_UNIFORM_BUFFER, clip_ubo_);
	gl::BufferData(GL_UNIFORM_BUFFER, sizeof(clip_ubo_data), nullptr, GL_DYNAMIC_DRAW);
	gl::BindBufferBase(GL_UNIFORM_BUFFER, clip_binding_point, clip_ubo_);

	glEnable(GL_BLEND);
	gl::BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	return true;
}

void rv::ogl_renderer::begin_frame_backend(const vector_2d<float> display_size) noexcept
{
	glViewport(0, 0, static_cast<GLsizei>(display_size.x), static_cast<GLsizei>(display_size.y));

	for (cstd::size_t i = 0; i < shader_count_; ++i)
	{
		gl::UseProgram(programs_[i]);
		gl::Uniform2f(uniforms_[i].display_size, display_size.x, display_size.y);
	}

	gl::UseProgram(0);
}

void rv::ogl_renderer::end_frame() noexcept
{
	flush_pending_vertices();
}

void rv::ogl_renderer::flush_pending_vertices() noexcept
{
	if (pending_vertices_.empty() || pending_indices_.empty())
	{
		return;
	}

	// pre-offset indices (client-side arrays, no glDrawElementsBaseVertex)
	adjusted_indices_.resize(pending_indices_.size());
	cstd::memcpy(adjusted_indices_.data(), pending_indices_.data(), pending_indices_.size() * sizeof(cstd::uint32_t));

	for (const auto& batch : pending_batches_)
	{
		if (batch.vertex_offset != 0)
		{
			for (cstd::uint32_t i = 0; i < batch.index_count; ++i)
			{
				adjusted_indices_[static_cast<cstd::size_t>(batch.index_offset) + i] += batch.vertex_offset;
			}
		}
	}

	// client-side vertex arrays
	gl::BindBuffer(GL_ARRAY_BUFFER, 0);
	gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	while (glGetError() != GL_NO_ERROR) {}

	const auto* vtx_base = reinterpret_cast<const char*>(pending_vertices_.data());
	constexpr GLsizei stride = sizeof(vertex);

	gl::EnableVertexAttribArray(0);
	gl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, vtx_base + offsetof(vertex, pos));

	gl::EnableVertexAttribArray(1);
	gl::VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, vtx_base + offsetof(vertex, col));

	gl::EnableVertexAttribArray(2);
	gl::VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, vtx_base + offsetof(vertex, uv));

	gl::EnableVertexAttribArray(3);
	gl::VertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, vtx_base + offsetof(vertex, custom_data));

	gl::EnableVertexAttribArray(4);
	gl::VertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, vtx_base + offsetof(vertex, custom_data) + 16);

	// batch iteration
	shader_type last_shader = static_cast<shader_type>(0xFF);
	GLuint last_tex_id = 0;
	optional_t<clip_rect_data> last_clip;
	bool clip_initialized = false;
	cstd::size_t current_program_idx = 0;

	const float display_h = state_.display_size.y;
	const bool use_ubo = (clip_ubo_ != 0);

	for (const auto& batch : pending_batches_)
	{
		if (batch.shader != last_shader)
		{
			last_shader = batch.shader;
			current_program_idx = static_cast<cstd::size_t>(batch.shader);
			gl::UseProgram(programs_[current_program_idx]);

			// GL2: uniforms are per-program, must re-send clip state
			if (!use_ubo)
			{
				clip_initialized = false;
			}
		}

		const auto tex = std::static_pointer_cast<ogl_texture>(batch.texture);
		const GLuint tex_id = tex->id();

		if (tex_id != last_tex_id)
		{
			last_tex_id = tex_id;
			gl::ActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex_id);
		}

		if (!clip_initialized || batch.clip_rect != last_clip)
		{
			clip_initialized = true;
			last_clip = batch.clip_rect;

			clip_ubo_data cb_data = { };

			if (batch.clip_rect.has_value())
			{
				const auto& clip = batch.clip_rect.value();

				cb_data.clip_min[0] = clip.bounds.min.x;
				cb_data.clip_min[1] = clip.bounds.min.y;
				cb_data.clip_max[0] = clip.bounds.max.x;
				cb_data.clip_max[1] = clip.bounds.max.y;
				cb_data.clip_enabled = 1.f;

				const float r = clip.rounding;
				cb_data.clip_radii[0] = (clip.flags & rounding_flags_top_right) ? r : 0.f;
				cb_data.clip_radii[1] = (clip.flags & rounding_flags_bottom_right) ? r : 0.f;
				cb_data.clip_radii[2] = (clip.flags & rounding_flags_bottom_left) ? r : 0.f;
				cb_data.clip_radii[3] = (clip.flags & rounding_flags_top_left) ? r : 0.f;
			}

			if (use_ubo)
			{
				// GL3: write to shared UBO
				gl::BindBuffer(GL_UNIFORM_BUFFER, clip_ubo_);
				gl::BufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(clip_ubo_data), &cb_data);
			}
			else
			{
				// GL2: set per-program uniforms
				const auto& u = uniforms_[current_program_idx];

				gl::Uniform2f(u.clip_min, cb_data.clip_min[0], cb_data.clip_min[1]);
				gl::Uniform2f(u.clip_max, cb_data.clip_max[0], cb_data.clip_max[1]);
				gl::Uniform1f(u.clip_enabled, cb_data.clip_enabled);
				gl::Uniform4f(u.clip_radii, cb_data.clip_radii[0], cb_data.clip_radii[1],
				              cb_data.clip_radii[2], cb_data.clip_radii[3]);
			}

			// scissor rect (Y-flipped for GL bottom-left origin)
			if (batch.clip_rect.has_value())
			{
				const GLint sx = static_cast<GLint>(batch.clip_rect->bounds.min.x) - 1;
				const GLint sy = static_cast<GLint>(display_h - batch.clip_rect->bounds.max.y) - 1;
				const GLsizei sw = static_cast<GLsizei>(batch.clip_rect->bounds.max.x - batch.clip_rect->bounds.min.x) + 2;
				const GLsizei sh = static_cast<GLsizei>(batch.clip_rect->bounds.max.y - batch.clip_rect->bounds.min.y) + 2;
				glScissor(sx, sy, sw, sh);
			}
			else
			{
				glScissor(0, 0, static_cast<GLsizei>(state_.display_size.x), static_cast<GLsizei>(state_.display_size.y));
			}
		}

		const auto* idx_ptr = adjusted_indices_.data() + batch.index_offset;
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(batch.index_count), GL_UNSIGNED_INT, idx_ptr);
	}

	gl::DisableVertexAttribArray(0);
	gl::DisableVertexAttribArray(1);
	gl::DisableVertexAttribArray(2);
	gl::DisableVertexAttribArray(3);
	gl::DisableVertexAttribArray(4);

	gl::UseProgram(0);

	if (pending_vertices_.size() > peak_vertex_count_)
		peak_vertex_count_ = pending_vertices_.size();
	if (pending_indices_.size() > peak_index_count_)
		peak_index_count_ = pending_indices_.size();
	if (pending_batches_.size() > peak_batch_count_)
		peak_batch_count_ = pending_batches_.size();

	pending_vertices_.clear();
	pending_indices_.clear();
	pending_batches_.clear();
}

shared_ptr_t<rv::texture> rv::ogl_renderer::create_texture(const span_t<const cstd::uint8_t> buffer,
                                                            const cstd::uint32_t width, const cstd::uint32_t height)
{
	if (buffer.empty() || !width || !height)
	{
		return { };
	}

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(width), static_cast<GLsizei>(height),
	             0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

	glBindTexture(GL_TEXTURE_2D, 0);

	return cstd::make_shared<ogl_texture>(this, tex);
}

shared_ptr_t<rv::texture> rv::ogl_renderer::create_texture_from_srv(void* raw_srv)
{
	if (!raw_srv)
	{
		return { };
	}

	const GLuint tex_id = static_cast<GLuint>(reinterpret_cast<cstd::size_t>(raw_srv));
	return cstd::make_shared<ogl_texture>(this, tex_id);
}
