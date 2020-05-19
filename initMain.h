
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
	flameEmitterProto->dispersion = 3.14159f;\
	flameEmitterProto->emission_rate = 3.f;\
	flameEmitterProto->lifetime = 0.67f;\
	flameEmitterProto->color = vec4(1, 1, 0.1f, 1.f);\
	flameEmitterProto->maxSpeed = 1.f;\
	flameEmitterProto->scale = vec3(1.f);\
	flameEmitterProto->billboard = 1;\
	flameEmitterProto->trail = 1;\
\
	emitter_prototype_ smokeEmitter = createNamedEmitter("smoke");\
	*smokeEmitter = *flameEmitterProto;\
	smokeEmitter->emission_rate = 2.f;\
	smokeEmitter->lifetime = 3.f;\
	smokeEmitter->color = vec4(0.5f, 0.5f, 0.5f, 0.2f);\
	smokeEmitter->maxSpeed = 1.f;\
	smokeEmitter->scale = vec3(1);\
	smokeEmitter->trail = 1;\
\
	emitter_prototype_ emitterProto2 = createNamedEmitter("expflame");\
	emitterProto2->dispersion = 3.14159f / 2.f;\
	emitterProto2->emission_rate = 50.f;\
	emitterProto2->lifetime = 1.f;\
	emitterProto2->color = vec4(1, 1, 0.2f, 0.8f);\
	emitterProto2->maxSpeed = 60.f;\
	emitterProto2->scale = vec3(25.f);\
	emitterProto2->trail = 0;\
	_expFlame = emitterProto2;\
\
	emitter_prototype_ emitterProto4 = createNamedEmitter("expsmoke");\
	*emitterProto4 = *emitterProto2;\
	emitterProto4->emission_rate = 50.f;\
	emitterProto4->lifetime = 6.f;\
	emitterProto4->scale = vec3(20);\
	emitterProto4->maxSpeed = (30.f);\
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
	laser.primarybullet->dispersion = 3.14159f;\
	laser.primarybullet->color = vec4(.8,.8,1,1);\
	laser.primarybullet->lifetime = 0.3f;\
	laser.primarybullet->emission_rate = 20.f;\
	laser.primarybullet->trail = 1;\
	laser.primarybullet->scale = vec3(20.f);\
	laser.primarybullet->maxSpeed = 1.f;\
	laser.secondarybullet = createNamedEmitter("laserbeam2");\
	laser.secondarybullet->dispersion = 3.14159f;\
	laser.secondarybullet->color = vec4(.6,.6,1,0.5);\
	laser.secondarybullet->lifetime = 1.f;\
	laser.secondarybullet->emission_rate = 20.f;\
	laser.secondarybullet->trail = 1;\
	laser.secondarybullet->scale = vec3(25.f);\
	laser.secondarybullet->maxSpeed = 1.f;\
	laser.primaryexplosion = createNamedEmitter("beamexplosion1");\
	laser.primaryexplosion->dispersion = 3.14159f;\
	laser.primaryexplosion->color = vec4(.8,.8,1,1);\
	laser.primaryexplosion->scale = vec3(1000);\
	laser.primaryexplosion->lifetime = 1.f;\
	laser.primaryexplosion->emission_rate = 50.f;\
	laser.primaryexplosion->trail = 0;\
	laser.primaryexplosion->maxSpeed = (2000.f);\
	laser.secondaryexplosion = createNamedEmitter("beamexplosion2");\
	laser.secondaryexplosion->dispersion = 3.14159f;\
	laser.secondaryexplosion->color = vec4(.2,.2,1,0.5);\
	laser.secondaryexplosion->scale = vec3(1000);\
	laser.secondaryexplosion->lifetime = 6.f;\
	laser.secondaryexplosion->emission_rate = 50.f;\
	laser.secondaryexplosion->trail = 0;\
	laser.secondaryexplosion->maxSpeed = (600.f);\
	bullets["laser"] = laser;\
\
	emitter_prototype_ engineTrail = createNamedEmitter("engineTrail");\
	*engineTrail = *flameEmitterProto;\
	engineTrail->dispersion = 0.0f;\
	engineTrail->emission_rate = 2.f;\
	engineTrail->lifetime = 10.f;\
	engineTrail->color = vec4(0.6f, 0.7f, 1.f, 0.6f);\
	engineTrail->maxSpeed = (0.f);\
	engineTrail->scale = vec3(1);\
	engineTrail->trail = 1;\
\
	emitter_prototype_ engineFlame = createNamedEmitter("engineFlame");\
	engineFlame->dispersion = 0.5f;\
	engineFlame->emission_rate = 7.f;\
	engineFlame->lifetime = 4.f;\
	engineFlame->color = vec4(0.5f, 0.5f, 0.9f, 0.2f);\
	engineFlame->maxSpeed = (-3.f);\
	engineFlame->scale = vec3(2.f);\
	engineFlame->trail = 0;\
\
emitter_prototype_ _muzzelFlash = createNamedEmitter("muzzelFlash");\
	_muzzelFlash->dispersion = 0.5f;\
	_muzzelFlash->emission_rate = 1.f;\
	_muzzelFlash->lifetime = 1.5f;\
	_muzzelFlash->color = vec4(1, 1, 0.2f, 0.8f);\
	_muzzelFlash->maxSpeed = (10.f);\
	_muzzelFlash->scale = vec3(3.f);\
	_muzzelFlash->trail = 0;\
\
	emitter_prototype_ _muzzelSmoke = createNamedEmitter("muzzelSmoke");\
	*_muzzelSmoke = *_muzzelFlash;\
	_muzzelSmoke->emission_rate = 1.f;\
	_muzzelSmoke->lifetime = 3.f;\
	_muzzelSmoke->scale = vec3(2.7);\
	_muzzelSmoke->maxSpeed = (5.f);\
	_muzzelSmoke->color = vec4(0.45f);\
	\
