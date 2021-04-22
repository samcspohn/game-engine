#include "lineRenderer.h"

struct line{
    glm::vec3 p1;
    int x;
    glm::vec3 p2;
    int x2;
    glm::vec4 color;
};

gpu_vector<line>* lines;
gpu_vector<line>* boxes;

void initLineRenderer(){
    lines = new gpu_vector<line>();
    lines->ownStorage();
    boxes = new gpu_vector<line>();
    boxes->ownStorage();
}

void addLine(glm::vec3 p1, glm::vec3 p2, glm::vec4 color){
    line l;
    l.p1 = p1;
    l.p2 = p2;
    l.color = color;
    lines->storage->push_back(l);
}
void addBox(glm::vec3 p, glm::vec3 dims, glm::vec4 color){
    line l;
    l.p1 = p;
    l.p2 = dims;
    l.color = color;
    boxes->storage->push_back(l);
}

void LineRendererBeginFrame(){
    lines->storage->clear();
    boxes->storage->clear();
}

void lineRendererRender(camera& c){

    static Shader lineShader("res/shaders/line.vert","res/shaders/line.geom","res/shaders/line.frag");
    static Shader boxShader("res/shaders/line.vert","res/shaders/line_box.geom","res/shaders/line.frag");
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

    glm::mat4 vp = c.proj * c.rot * glm::translate(-c.pos);

    // lineshader set up variables
    lineShader.use();
    lineShader.setVec3("camDir",c.dir);
    lineShader.setMat4("vp",vp);
    lineShader.setFloat("FC", 2.0 / log2(c.farPlane + 1));

    glBindVertexArray(VAO);

    glDrawArrays(GL_POINTS, 0, lines->storage->size());

    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindVertexArray(0);

    boxes->bufferData();
    boxes->bindData(0);
    boxShader.use();
    boxShader.setVec3("camDir",c.dir);
    boxShader.setMat4("vp",vp);
    boxShader.setFloat("FC", 2.0 / log2(c.farPlane + 1));

    glBindVertexArray(VAO);

    glDrawArrays(GL_POINTS, 0, boxes->storage->size());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}