#include "lineRenderer.h"

struct line{
    glm::vec3 p1;
    int x;
    glm::vec3 p2;
    int x2;
    glm::vec4 color;
};

gpu_vector<line>* lines;

void initLineRenderer(){
    lines = new gpu_vector<line>();
    lines->ownStorage();
}

void addLine(glm::vec3 p1, glm::vec3 p2, glm::vec4 color){
    line l;
    l.p1 = p1;
    l.p2 = p2;
    l.color = color;
    lines->storage->push_back(l);
}

void LineRendererBeginFrame(){
    lines->storage->clear();
}

void lineRendererRender(camera& c){

    static Shader lineShader("res/shaders/line.vert","res/shaders/line.geom","res/shaders/line.frag");
    static GLuint VAO = 0;

    if (VAO == 0)
    {
        glGenVertexArrays(1, &VAO);
    }
    lines->bufferData();
    lines->bindData(0);

	// glDisable(GL_DEPTH_TEST);
	// glDisable(GL_CULL_FACE);
	// // glEnable(GL_DEPTH_TEST);
	// glDepthMask(GL_FALSE);
	// glDepthMask(GL_FALSE);
	// glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	// glBlendEquation(GL_FUNC_ADD);

    // lineshader set up variables
    lineShader.use();
    lineShader.setVec3("camDir",c.dir);
    glm::mat4 vp = c.proj * c.rot * glm::translate(-c.pos);
    lineShader.setMat4("vp",vp);
    lineShader.setFloat("FC", 2.0 / log2(c.farPlane + 1));

    glBindVertexArray(VAO);

    glDrawArrays(GL_POINTS, 0, lines->storage->size());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}