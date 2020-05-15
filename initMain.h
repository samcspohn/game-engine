
#define initmain()\
	_shader particleShader("res/shaders/particles.vert", "res/shaders/particles.geom", "res/shaders/particles.frag");\
	_shader modelShader("res/shaders/model.vert", "res/shaders/model.frag");\
	_model cubeModel("res/models/cube/cube.obj");\
	_model nanoSuitModel("res/models/nanosuit/nanosuit.obj");\
	_model terrainModel("res/models/terrain/terrain.obj");\
\
	collisionGraph[0] = {1};\
	collisionGraph[1] = {0,1};\
\
	emitter_prototype_ flameEmitterProto = createNamedEmitter("flame");\
	flameEmitterProto->emission_rate = 3.f;\
	flameEmitterProto->lifetime = 0.67f;\
	flameEmitterProto->color = vec4(1, 1, 0.1f, 0.8f);\
	flameEmitterProto->velocity = vec3(1.f);\
	flameEmitterProto->scale = vec3(2.f);\
	flameEmitterProto->billboard = 1;\
	flameEmitterProto->trail = 1;\
\
	emitter_prototype_ smokeEmitter = createNamedEmitter("smoke");\
	*smokeEmitter = *flameEmitterProto;\
	smokeEmitter->emission_rate = 2.f;\
	smokeEmitter->lifetime = 3.f;\
	smokeEmitter->color = vec4(0.5f, 0.5f, 0.5f, 0.2f);\
	smokeEmitter->velocity = vec3(4.f);\
	smokeEmitter->scale = vec3(3);\
	smokeEmitter->trail = 1;\
\
	emitter_prototype_ emitterProto2 = createNamedEmitter("expflame");\
	emitterProto2->emission_rate = 50.f;\
	emitterProto2->lifetime = 1.f;\
	emitterProto2->color = vec4(1, 1, 0.2f, 0.8f);\
	emitterProto2->velocity = vec3(60.f);\
	emitterProto2->scale = vec3(25.f);\
	emitterProto2->trail = 0;\
	_expFlame = emitterProto2;\
\
	emitter_prototype_ emitterProto4 = createNamedEmitter("expsmoke");\
	*emitterProto4 = *emitterProto2;\
	emitterProto4->emission_rate = 50.f;\
	emitterProto4->lifetime = 6.f;\
	emitterProto4->scale = vec3(20);\
	emitterProto4->velocity = vec3(30.f);\
	emitterProto4->color = vec4(0.45f);\
	_expSmoke = emitterProto4;\
	\
	bullet bomb;\
	bomb.primarybullet = flameEmitterProto;\
	bomb.secondarybullet = smokeEmitter;\
	bomb.primaryexplosion = emitterProto2;\
	bomb.secondaryexplosion = emitterProto4;\
	bullets["bomb"] = bomb;\
\
	bullet laser;\
	laser.primarybullet = createNamedEmitter("laserbeam");\
	laser.primarybullet->color = vec4(.8,.8,1,1);\
	laser.primarybullet->lifetime = 0.3f;\
	laser.primarybullet->emission_rate = 20.f;\
	laser.primarybullet->trail = 1;\
	laser.primarybullet->scale = vec3(20.f);\
	laser.primarybullet->velocity = vec3(1.f);\
	laser.secondarybullet = createNamedEmitter("laserbeam2");\
	laser.secondarybullet->color = vec4(.6,.6,1,0.5);\
	laser.secondarybullet->lifetime = 1.f;\
	laser.secondarybullet->emission_rate = 20.f;\
	laser.secondarybullet->trail = 1;\
	laser.secondarybullet->scale = vec3(25.f);\
	laser.secondarybullet->velocity = vec3(1.f);\
	laser.primaryexplosion = createNamedEmitter("beamexplosion1");\
	laser.primaryexplosion->color = vec4(.8,.8,1,1);\
	laser.primaryexplosion->scale = vec3(1000);\
	laser.primaryexplosion->lifetime = 1.f;\
	laser.primaryexplosion->emission_rate = 50.f;\
	laser.primaryexplosion->trail = 0;\
	laser.primaryexplosion->velocity = vec3(2000.f);\
	laser.secondaryexplosion = createNamedEmitter("beamexplosion2");\
	laser.secondaryexplosion->color = vec4(.2,.2,1,0.5);\
	laser.secondaryexplosion->scale = vec3(1000);\
	laser.secondaryexplosion->lifetime = 6.f;\
	laser.secondaryexplosion->emission_rate = 50.f;\
	laser.secondaryexplosion->trail = 0;\
	laser.secondaryexplosion->velocity = vec3(600.f);\
	bullets["laser"] = laser;\
\
	bullet blackLaser;\
	blackLaser.primarybullet = createNamedEmitter("blacklaserbeam");\
	blackLaser.primarybullet->color = vec4(0,0,0,1);\
	blackLaser.primarybullet->lifetime = 0.7f;\
	blackLaser.primarybullet->emission_rate = 20.f;\
	blackLaser.primarybullet->trail = 1;\
	blackLaser.primarybullet->scale = vec3(100.f);\
	blackLaser.primarybullet->velocity = vec3(1.f);\
	blackLaser.secondarybullet = createNamedEmitter("blacklaserbeam2");\
	blackLaser.secondarybullet->color = vec4(1,0,0,0.5);\
	blackLaser.secondarybullet->lifetime = 1.f;\
	blackLaser.secondarybullet->emission_rate = 20.f;\
	blackLaser.secondarybullet->trail = 1;\
	blackLaser.secondarybullet->scale = vec3(160.f);\
	blackLaser.secondarybullet->velocity = vec3(1.f);\
	blackLaser.primaryexplosion = createNamedEmitter("blackbeamexplosion1");\
	blackLaser.primaryexplosion->color = vec4(0,0,0,1);\
	blackLaser.primaryexplosion->scale = vec3(10000);\
	blackLaser.primaryexplosion->lifetime = 3.f;\
	blackLaser.primaryexplosion->emission_rate = 100.f;\
	blackLaser.primaryexplosion->trail = 0;\
	blackLaser.primaryexplosion->velocity = vec3(10000.f);\
	blackLaser.secondaryexplosion = createNamedEmitter("blackbeamexplosion2");\
	blackLaser.secondaryexplosion->color = vec4(0.8,0,0,0.5);\
	blackLaser.secondaryexplosion->scale = vec3(15000);\
	blackLaser.secondaryexplosion->lifetime = 6.f;\
	blackLaser.secondaryexplosion->emission_rate = 40.f;\
	blackLaser.secondaryexplosion->trail = 0;\
	blackLaser.secondaryexplosion->velocity = vec3(5000.f);\
	bullets["black"] = blackLaser;\
