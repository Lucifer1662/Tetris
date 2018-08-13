#pragma once
#include <GL\glew.h>
#include <vector>



class Buffer {

	unsigned int bo;
	unsigned int size;
	unsigned int usage;
	unsigned int type;
	Buffer(const Buffer& buffer) = delete;
public:
	void SetUsage(unsigned int usage) {
		Buffer::usage = usage;
	}

	void CreateBuffer() {
		if (bo == -1) {
			glGenBuffers(1, &bo);
		}
	}
	
	Buffer(unsigned int type, unsigned int usage = GL_DYNAMIC_DRAW, bool createBuffer = true):type(type) {
		SetUsage(usage);
		if (createBuffer)
			glGenBuffers(1, &bo);
		else
			bo = -1;
	}

	Buffer(void* data, unsigned int sizeInBytes, unsigned int usage = GL_DYNAMIC_DRAW) : Buffer(type, usage) {
		SetData(data, sizeInBytes);
	}

	Buffer(Buffer&& o) {
		*this = std::move(o);
	}

	Buffer&& CreateCopy() const {
		Buffer buffer(type);
		//create a temp buffer of type GL_COPY_READ_BUFFER
		buffer.Bind(GL_COPY_READ_BUFFER);
		glBufferData(GL_COPY_READ_BUFFER, size, NULL, GL_STATIC_COPY);
		buffer.size = size;

		//copy data to buffer
		Bind();
		glCopyBufferSubData(type, GL_COPY_READ_BUFFER, 0, 0, size);

		return std::move(buffer);
	}
	
	inline void Bind()const {
		Bind(type);
	}

	inline void Bind(unsigned bufferType)const {
		glBindBuffer(bufferType, bo);
	}

	inline void BindBufferBase(int i) const {
		Bind();
		glBindBufferBase(type, i, bo);
	}

	void SetData(void* data, unsigned int sizeInBytes) {
		Bind();
		size = sizeInBytes;
		glBufferData(type, sizeInBytes, data, usage);
	}
	void SetSubData(void* data, unsigned int sizeInBytes, unsigned int offset) {
		if (offset + sizeInBytes > size)
			ResizeBuffers(sizeInBytes, offset);
		Bind();		
		glBufferSubData(type, offset, sizeInBytes, data);
		
	}

	int GetData(void* data) const{
		Bind();
		glGetBufferSubData(type, 0, size, data);
		return size;
	}

	int GetSubData(void* data, unsigned offset, unsigned int size) const {
		Bind();
	
		glGetBufferSubData(type, offset, size, data);
		return size;
	}

	Buffer& operator=(const Buffer& buffer) = delete;

	Buffer& operator=(Buffer&& buffer) {
		if(bo != -1)
			glDeleteBuffers(1, &bo);
		bo = buffer.bo;
		size = buffer.size;
		usage = buffer.usage;
		buffer.bo = 0;
		buffer.size = 0;
		return *this;
	}

	void ResizeBuffers(unsigned int newSize) {
		ResizeBuffers(newSize, size);
	}

	void ResizeBuffers(unsigned int newSize, unsigned int copyAmount) {
		unsigned int vbotemp;
		glGenBuffers(1, &vbotemp);
		Bind();

		if (copyAmount > 0) {
			//create a temp buffer of type GL_COPY_READ_BUFFER
			
			glBindBuffer(GL_COPY_READ_BUFFER, vbotemp);
			glBufferData(GL_COPY_READ_BUFFER, newSize, NULL, GL_STATIC_COPY);

			//copy data from vbo1 to it
			glCopyBufferSubData(type, GL_COPY_READ_BUFFER, 0, 0, copyAmount);
			glBindBuffer(type, vbotemp);
			glDeleteBuffers(1, &bo);
		}
		else {
			glBindBuffer(type, vbotemp);
			glBufferData(type, newSize, NULL, usage);
		}
		bo = vbotemp;
		size = newSize;
		Bind();
	}

	inline ~Buffer() {
		glDeleteBuffers(1, &bo);
	}


};
