#include "SceneManager.h"


#define DEG_TO_RADIAN 0.017453293

SceneManager::SceneManager() {
	eye = glm::vec3(0.0f, 1.0f, 0.0f);
	at = glm::vec3(0.0f, 1.0f, -1.0f);
	up = glm::vec3(0.0f, 1.0f, 0.0f);
	skyboxProgram = rt3d::initShaders("cubeMap.vert", "cubeMap.frag");

	//lights - initialise first light - can possibly be read in from file using rt3d::load file
	lights.push_back({
		{ 0.3f, 0.3f, 0.3f, 1.0f },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		{ -10.0f, 10.0f, 10.0f, 1.0f } });

	//materials - same as above
	materials.push_back({
		{ 0.0f, 0.0f, 1.0f, 1.0f }, // ambient
		{ 0.5f, 1.0f, 0.5f, 1.0f }, // diffuse
		{ 0.0f, 0.1f, 0.0f, 1.0f }, // specular
		2.0f  // shininess
	}
	);

	lightPos = { 0.0f, 2.0f, -6.0f, 1.0f };
}

glm::vec3 SceneManager::moveForward(glm::vec3 pos, GLfloat angle, GLfloat d)
{
	return glm::vec3(pos.x + d*std::sin(angle*DEG_TO_RADIAN), pos.y, pos.z - d*std::cos(angle*DEG_TO_RADIAN));
}

glm::vec3 SceneManager::moveRight(glm::vec3 pos, GLfloat angle, GLfloat d)
{
	return glm::vec3(pos.x + d*std::cos(angle*DEG_TO_RADIAN), pos.y, pos.z + d*std::sin(angle*DEG_TO_RADIAN));
}

void SceneManager::renderSkybox(glm::mat4 projection)
{
	// skybox as single cube using cube map
	glUseProgram(skyboxProgram);
	rt3d::setUniformMatrix4fv(skyboxProgram, "projection", glm::value_ptr(projection));
	glDepthMask(GL_FALSE); // make sure writing to update depth test is off
	glm::mat3 mvRotOnlyMat3 = glm::mat3(mvStack.top());
	mvStack.push(glm::mat4(mvRotOnlyMat3));
	glCullFace(GL_FRONT); // drawing inside of cube!
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox[0]);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(1.5f, 1.5f, 1.5f));
	rt3d::setUniformMatrix4fv(skyboxProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();
	glCullFace(GL_BACK); // drawing inside of cube!
						 // back to remainder of rendering
	glDepthMask(GL_TRUE); // make sure depth test is on
}

void SceneManager::clearScreen()
{
	// clear the screen
	glEnable(GL_CULL_FACE);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

glm::mat4 SceneManager::initRendering()
{
	glm::mat4 projection(1.0);
	projection = glm::perspective(float(60.0f*DEG_TO_RADIAN), 800.0f / 600.0f, 1.0f, 150.0f);

	//GLfloat scale(1.0f); // just to allow easy scaling of complete scene

	glm::mat4 modelview(1.0); // set base position for scene
	mvStack.push(modelview);

	initCamera();

	return projection;
}

void SceneManager::initCamera() {
	//init camera???
	at = player.getPlayerPos();
	eye = moveForward(at, player.getPlayerR(), -8.0f);
	eye.y += 3.0;
	mvStack.top() = glm::lookAt(eye, at, up);
}

void SceneManager::init()
{
	shaderProgram = rt3d::initShaders("phong-tex.vert", "phong-tex.frag");
	rt3d::setLight(shaderProgram, lights[0]);
	rt3d::setMaterial(shaderProgram, materials[0]);

	//matching textureUnits
	GLuint uniformIndex = glGetUniformLocation(shaderProgram, "textureUnit0");
	glUniform1i(uniformIndex, 0);
	uniformIndex = glGetUniformLocation(shaderProgram, "textureUnit1");
	glUniform1i(uniformIndex, 1);
	uniformIndex = glGetUniformLocation(shaderProgram, "textureUnit2");
	glUniform1i(uniformIndex, 2);

	//skybox program needed in draw method
	//GLuint skyboxProgram = rt3d::initShaders("cubeMap.vert", "cubeMap.frag");

	//doesn't appear to be used anywhere else question mark question mark question mark
	GLuint textureProgram = rt3d::initShaders("textured.vert", "textured.frag");

	//loading skybox
	const char *cubeTexFiles[6] = {
		"Town-skybox/grass1.bmp", "Town-skybox/side1.bmp", "Town-skybox/grass1.bmp", "Town-skybox/grass1.bmp", "Town-skybox/grass1.bmp", "Town-skybox/grass1.bmp"
	};

	SDLManager::loadCubeMap(cubeTexFiles, skybox);

	// its own method to load cubes more easily???
	std::vector<GLfloat> verts;
	std::vector<GLfloat> norms;
	std::vector<GLfloat> tex_coords;
	std::vector<GLuint> indices;
	rt3d::loadObj("cube.obj", verts, norms, tex_coords, indices);
	meshIndexCount = indices.size();

	textures.push_back(SDLManager::loadBitmap("fabric.bmp"));

	meshObjects.push_back(rt3d::createMesh(verts.size() / 3,
		verts.data(), nullptr, norms.data(),
		tex_coords.data(), meshIndexCount,
		indices.data()));

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void SceneManager::setShaderProjection(glm::mat4 projection)
{
	glUseProgram(shaderProgram);
	rt3d::setUniformMatrix4fv(shaderProgram, "projection", glm::value_ptr(projection));
}

void SceneManager::setLights()
{
	glm::vec4 tmp = mvStack.top()*lightPos;
	lights[0].position[0] = tmp.x;
	lights[0].position[1] = tmp.y;
	lights[0].position[2] = tmp.z;
	rt3d::setLightPos(shaderProgram, glm::value_ptr(tmp));
}

void SceneManager::renderObjects()
{
	//cube - ground plane
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(-5.0f, -0.1f, -100.0f));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(20.0f, 0.1f, 200.0f));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, materials[0]);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	//player cube
	///glUseProgram(shaderProgram);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(player.getPlayerPos().x, player.getPlayerPos().y, player.getPlayerPos().z));
	mvStack.top() = glm::rotate(mvStack.top(), float(player.getPlayerR()*DEG_TO_RADIAN), glm::vec3(0.0f, 1.0f, 0.0f));
													// ^^ 180 was 'r'
	mvStack.top() = glm::rotate(mvStack.top(), float(180 * DEG_TO_RADIAN), glm::vec3(1.0f, 0.0f, 0.0f));
	mvStack.top() = glm::rotate(mvStack.top(), float(180 * DEG_TO_RADIAN), glm::vec3(0.0f, 0.0f, 1.0f));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, materials[0]);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();
}

void SceneManager::updatePlayerR(GLfloat deltaR)
{
	player.setPlayerR(player.getPlayerR() - deltaR);
}

void SceneManager::renderObject()
{
}
