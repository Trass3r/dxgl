#pragma once

// Note: OpenGL #includes deliberately absent

#include <initializer_list>
#include <memory>
#include <string_view>
#include <type_traits>
#include <assert.h>

struct GLHandle
{
	GLuint handle = 0;
	GLHandle() = default;
	GLHandle(const GLHandle&) = delete;
	void operator=(const GLHandle&) = delete;
	operator GLuint() const { return handle; }
};

struct VAO final : GLHandle
{
	VAO(std::string_view name)
	{
		glGenVertexArrays(1, &handle);
		glBindVertexArray(handle);
		glObjectLabel(GL_VERTEX_ARRAY, handle, (GLsizei)name.size(), name.data());
	}
	~VAO() { glDeleteVertexArrays(1, &handle); }
	void bind() const { glBindVertexArray(handle); }
};

struct GLBuffer final : GLHandle
{
	GLenum target = 0;
	GLBuffer(GLenum target, std::string_view name)
	: target(target)
	{
		glGenBuffers(1, &handle);
		glBindBuffer(target, handle);
		glObjectLabel(GL_BUFFER, handle, (GLsizei)name.size(), name.data());
	}
	~GLBuffer() { glDeleteBuffers(1, &handle); }
	void bind() const { glBindBuffer(target, handle); }
	void unbind() const { glBindBuffer(target, 0); }
	void bindBase(GLuint index) const { glBindBufferBase(target, index, handle); }
	void bindRange(GLuint index, size_t offset, size_t size) const { glBindBufferRange(target, index, handle, (GLintptr)offset, (GLsizeiptr)size); }
	void setStorage(size_t size, const void* data, GLbitfield flags) { glBufferStorage(target, (GLsizeiptr)size, data, flags); }
	void setData(size_t size, const void* data, GLenum usage) { glBufferData(target, (GLsizeiptr)size, data, usage); }
	void setSubData(size_t offset, size_t size, const void* data) { glBufferSubData(target, (GLintptr)offset, (GLsizeiptr)size, data); }
	void map(GLenum access) { glMapBuffer(target, access); }
	void mapRange(size_t offset, size_t length, GLbitfield access) { glMapBufferRange(target, (GLintptr)offset, (GLsizeiptr)length, access); }
	void unmap(GLenum access) { glUnmapBuffer(target); }
};

struct GLTexture final : GLHandle
{
	const GLenum target = 0;
	GLTexture(GLenum target, std::string_view name, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height = 1, GLsizei depth = 1)
	: target(target)
	{
		glGenTextures(1, &handle);
		glBindTexture(target, handle);
		glObjectLabel(GL_TEXTURE, handle, (GLsizei)name.size(), name.data());
		if (depth != 1)
			glTexStorage3D(target, levels, internalformat, width, height, depth);
		else if (height != 1)
			glTexStorage2D(target, levels, internalformat, width, height);
		else
			glTexStorage1D(target, levels, internalformat, width);
	}
	~GLTexture() { glDeleteTextures(1, &handle); }
	void bind() const { glBindTexture(target, handle); }

	void setSubData(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* data)
	{
		glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, data);
	}

	void generateMipmaps() { glGenerateMipmap(target); }

	template <typename T, typename = std::enable_if_t<std::is_same_v<T, int> || std::is_same_v<T, float>>>
	[[nodiscard]]
	T get(GLenum name) const
	{
		assert(name != GL_TEXTURE_BORDER_COLOR);
		T value = 0; // init is required
		if constexpr (std::is_integral_v<T>)
			glGetTexParameteriv(target, name, &value);
		else
			glGetTexParameterfv(target, name, &value);

		return value;
	}

	void set(GLenum name, GLint value)   { glTexParameteri(target, name, value); }
	void set(GLenum name, GLfloat value) { glTexParameterf(target, name, value); }
};

struct GLFBO final : GLHandle
{
	mutable GLenum target = 0; // GL_FRAMEBUFFER, GL_READ_FRAMEBUFFER or GL_DRAW_FRAMEBUFFER
	GLFBO(GLenum target, std::string_view name, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height = 1, GLsizei depth = 1)
	: target(target)
	{
		glGenFramebuffers(1, &handle);
		glBindFramebuffer(target, handle);
		glObjectLabel(GL_FRAMEBUFFER, handle, (GLsizei)name.size(), name.data());
		if (depth != 1)
			glTexStorage3D(target, levels, internalformat, width, height, depth);
		else if (height != 1)
			glTexStorage2D(target, levels, internalformat, width, height);
		else
			glTexStorage1D(target, levels, internalformat, width);
	}
	~GLFBO() { glDeleteFramebuffers(1, &handle); }

	void bind(GLenum tgt) const { target = tgt; glBindFramebuffer(target, handle); }
	void unbind() const { glBindFramebuffer(target, 0); target = 0; }

	// TODO: renderbuffer, array textures, cubemaps, 3D textures
	void attach(GLenum attachment, GLuint texture, int level) { glFramebufferTexture(target, attachment, texture, level); }
	void detach(GLenum attachment) { attach(attachment, 0, 0); }
	[[nodiscard]]
	GLenum status() const { return glCheckFramebufferStatus(target); }
	[[nodiscard]]
	bool complete() const { return status() == GL_FRAMEBUFFER_COMPLETE; }
};

struct GLSLShader final : GLHandle
{
	GLSLShader(GLenum type, std::string_view name, const char* code)
	{
		handle = glCreateShader(type);
		glObjectLabel(GL_SHADER, handle, (GLsizei)name.size(), name.data());
		glShaderSource(handle, 1, &code, nullptr);
		if (!compile())
			printf("LOG: %s\n", getLog().get());
	}

	~GLSLShader() { glDeleteShader(handle); }
	bool compile()
	{
		glCompileShader(handle);
		GLint status;
		glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
		return status != GL_FALSE;
	}
	// avoids std::string since it is a huge include and would double allocate
	std::unique_ptr<char[]> getLog() const
	{
		int logLength;
		glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLength);
		std::unique_ptr<char[]> log;
		if (logLength > 0)
		{
			log.reset(new char[(size_t)logLength]);
			glGetShaderInfoLog(handle, logLength, nullptr, log.get());
		}
		return std::move(log);
	}
};

struct GLSLProgram final : GLHandle
{
	GLSLProgram(std::string_view name, std::initializer_list<GLSLShader*> shaders)
	{
		handle = glCreateProgram();
		glObjectLabel(GL_PROGRAM, handle, (GLsizei)name.size(), name.data());
		for (GLSLShader* shader : shaders)
			glAttachShader(handle, *shader);
		if (!link())
			printf("LOG: %s\n", getLog().get());
		else
			use();
		for (GLSLShader* shader : shaders)
			glDetachShader(handle, *shader);
	}
	~GLSLProgram() { glDeleteProgram(handle); }
	void use() const { glUseProgram(handle); }
	bool link()
	{
		glLinkProgram(handle);
		glValidateProgram(handle);
		// have to check status as glLinkProgram always returns "no error"
		GLint status;
		glGetProgramiv(handle, GL_LINK_STATUS, &status);
		return status != 0;
	}

	// avoids std::string since it is a huge include and would double allocate
	std::unique_ptr<char[]> getLog() const
	{
		int logLength;
		glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &logLength);
		std::unique_ptr<char[]> log;
		if (logLength > 0)
		{
			log.reset(new char[(size_t)logLength]);
			glGetProgramInfoLog(handle, logLength, nullptr, log.get());
		}
		return std::move(log);
	}
};

static const char* debugSourceString(GLenum source)
{
	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		return "API";
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		return "Window System";
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		return "Shader Compiler";
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		return "Third Party";
	case GL_DEBUG_SOURCE_APPLICATION:
		return "Application";
	case GL_DEBUG_SOURCE_OTHER:
		return "Other";
	default:
		return "UNKNOWN";
	}
}

static const char* debugTypeString(GLenum type)
{
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		return "Error";
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		return "Deprecated";
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		return "Undefined";
	case GL_DEBUG_TYPE_PORTABILITY:
		return "Portability";
	case GL_DEBUG_TYPE_PERFORMANCE:
		return "Performance";
	case GL_DEBUG_TYPE_OTHER:
		return "Other";
	case GL_DEBUG_TYPE_MARKER:
		return "Marker";
	default:
		return "UNKNOWN";
	}
}

static const char* debugSeverityString(GLenum severity)
{
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		return "High";
	case GL_DEBUG_SEVERITY_MEDIUM:
		return "Medium";
	case GL_DEBUG_SEVERITY_LOW:
		return "Low";
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		return "Notification";
	default:
		return "UNKNOWN";
	}
}

static void setupStderrDebugCallback()
{
	auto callback = [](GLenum source,
	                   GLenum type,
	                   GLuint /*id*/,
	                   GLenum severity,
	                   GLsizei length,
	                   const GLchar* message,
	                   const void*) {
		fprintf(stderr, "[%s] %s (%s): %.*s\n",
		    debugSourceString(source),
		    debugTypeString(type),
		    debugSeverityString(severity),
		    length, message);
		if (type == GL_DEBUG_TYPE_ERROR || severity == GL_DEBUG_SEVERITY_HIGH)
			exit(1);
	};
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(callback, nullptr);
}

struct GLScopedDebugMarker
{
	#ifndef NDEBUG
	explicit GLScopedDebugMarker(const char* msg) { glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, msg); }
	~GLScopedDebugMarker() { glPopDebugGroup(); }
	#else
	explicit GLScopedDebugMarker(const char* msg) {}
	~GLScopedDebugMarker() {}
#endif
};
