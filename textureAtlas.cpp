
#include "textureAtlas.h"
texAtlas::texAtlas(string name)
{
    atlas._new();
}

bool texAtlas::addTexture(_texture _t)
{
    // if(x > sz.x || y > sz.y){
    // resize texture;
    // }
    if(textures.find(_t) != textures.end())
        return false;
    // int tex_size = textures.size();

    _waitForRenderJob(new std::function<void()>([&]()
                                                {
                                                    sz.x = std::max(sz.x, _t.meta()->dims.x);
                                                    sz.y = sz.y + _t.meta()->dims.y;
                                                    vector<glm::u8vec4> pixels(sz.x * sz.y);
                                                    atlas.meta()->gen(sz.x, sz.y);

                                                    textures.emplace(_t);

                                                    int y_axis = 0;
                                                    auto xy = [&](int a, int b)
                                                    {
                                                        return a + b * sz.x;
                                                    };
                                                    for (auto &t : textures)
                                                    {
                                                        vector<glm::u8vec4> colors;
                                                        colors.resize(t.meta()->dims.x * t.meta()->dims.y);
                                                        t.meta()->read(colors.data());
                                                        // for(auto& color : colors){
                                                        //     color.r = color.g = color.b = 255;
                                                        // }
                                                        uvMap[t].coord = {0.f, float(y_axis) / float(sz.y)};
                                                        uvMap[t].sz = {float(t.meta()->dims.x) / float(sz.x), float(t.meta()->dims.y) / float(sz.y)};
                                                        // uvMap[t].coord.x += 0.5 / sz.x;
                                                        // uvMap[t].coord.y += 0.5 / sz.y;
                                                        // uvMap[t].sz.x += 0.5 / sz.x;
                                                        // uvMap[t].sz.y += 0.5 / sz.y;

                                                        auto _xy = [&](int a, int b)
                                                        {
                                                            return a + b * t.meta()->dims.x;
                                                        };
                                                        for (int i = 0; i < t.meta()->dims.x; i++)
                                                        {
                                                            for (int j = 0; j < t.meta()->dims.y; j++)
                                                            {
                                                                pixels[xy(i, y_axis + j)] = colors[_xy(i, j)];
                                                            }
                                                        }
                                                        y_axis += t.meta()->dims.y;
                                                    }
                                                    // atlas.t->write(pixels.data());
                                                    if (atlas.meta()->glid != -1)
                                                    {
                                                        glDeleteTextures(1, &atlas.meta()->glid);
                                                    }
                                                    glGenTextures(1, &atlas.meta()->glid);
                                                    glBindTexture(GL_TEXTURE_2D, atlas.meta()->glid);
                                                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sz.x, sz.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                                                    glGenerateMipmap(GL_TEXTURE_2D);

                                                    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 7);
                                                    // Parameters
                                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                                                    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
                                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                                                    glBindTexture(GL_TEXTURE_2D, 0);
                                                }));
    return true;
    // return tex_size != textures.size();
}
