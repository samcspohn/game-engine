#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <map>
enum renderTextureType{
	FLOAT,
	UNSIGNED_BYTE
};
struct tex{
	renderTextureType type;
	GLuint id;
	GLuint location;
};
class renderTexture{
public:
	int scr_width = 1;
	int scr_height = 1;
	GLuint FramebufferName = 0;
	std::map<string, tex> textures;
	unsigned int rboDepth = -1;

	GLuint getTexture(string name){
		return textures.at(name).id;
	}
	bool resize(int width, int height){
		if(width != scr_width || height != scr_height){
			scr_width = width;
			scr_height = height;
			for(auto& i : textures){
				// "Bind" the newly created texture : all future texture functions will modify this texture
				glBindTexture(GL_TEXTURE_2D, i.second.id);

				switch (i.second.type)
				{
					case renderTextureType::FLOAT:
						glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB16F, scr_width, scr_height, 0,GL_RGBA, GL_FLOAT, 0);
						break;
					case renderTextureType::UNSIGNED_BYTE:
						glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, scr_width, scr_height, 0,GL_RGBA, GL_UNSIGNED_BYTE, 0);
						break;
					default:
						break;
				}

				// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				// glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i.second.location, i.second.id, 0);
			}
			if(rboDepth != -1){
				glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, scr_width, scr_height);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
			}
			return true;
		}
		return false;
	}
	void addColorAttachment(string name, renderTextureType type, GLuint location){
		
		textures[name];
		glGenTextures(1, &textures[name].id);

		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, textures[name].id);
		textures[name].type = type;
		textures[name].location = location;

		// Give an empty image to OpenGL ( the last "0" )
		switch (type)
		{
		case renderTextureType::FLOAT:
			glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB32F, scr_width, scr_height, 0,GL_RGBA, GL_FLOAT, 0);
			break;
		case renderTextureType::UNSIGNED_BYTE:
			glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, scr_width, scr_height, 0,GL_RGBA, GL_UNSIGNED_BYTE, 0);
			break;
		default:
			break;
		}

		// Poor filtering. Needed !
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + location, textures[name].id, 0);

	}
	void addDepthBuffer(){
		glGenRenderbuffers(1, &rboDepth);
		glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, scr_width, scr_height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	}
	
	void finalize(){
		vector<GLenum> drawBuffers;
		for(int i = 0; i < textures.size(); i++){
			drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
		}
		glDrawBuffers(drawBuffers.size(), drawBuffers.data());
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        	std::cout << "Framebuffer not complete!" << std::endl;
    	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
    void init(){
        // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
		glGenFramebuffers(1, &FramebufferName);
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

		
    }
    void use(){
        /// framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		// vector<GLenum> drawBuffers;
		// for(auto& i : textures){
		// 	drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i.second.location);
		// }
		// glDrawBuffers(drawBuffers.size(), drawBuffers.data());
    }
    void blit(GLuint dest, int dest_width, int dest_height){
        glBindFramebuffer( GL_READ_FRAMEBUFFER, FramebufferName);
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, dest);

        glBlitFramebuffer(0, 0, scr_width, scr_height, 0, 0, dest_width, dest_height,
                GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
	void blitDepth(GLuint dest, int dest_width, int dest_height){
        glBindFramebuffer( GL_READ_FRAMEBUFFER, FramebufferName);
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, dest);

        glBlitFramebuffer(0, 0, scr_width, scr_height, 0, 0, dest_width, dest_height,
                GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        // glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};