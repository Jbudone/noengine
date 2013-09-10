#include "kernel/k_world.h"


World::World(bool renderable)
 : renderable(renderable) {
	shadermgr = 0;
}

// ============================================== //
// Load the world along with all of its properties, entities, textures, etc.
void World::loadWorld() {

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Setup shaders
	ShaderProgram* program = 0;
	RenderGroup* renderer = 0;
	if ( renderable ) {
		shadermgr = new ShaderManager();


		// load individual shaders properly
		program = shadermgr->createProgram();
		glUseProgram( program->programid );
		ShaderSet *shader = new ShaderSet();
		if ( shadermgr->loadShader( program, "data/shaders/shade.vert", GL_VERTEX_SHADER, SHD_PHONG | SHD_BUMP, &shader ) & ERROR ) { throw exception(); }
		Log( "Loaded vert shader" );
		if ( shadermgr->loadShader( program, "data/shaders/shade.frag", GL_FRAGMENT_SHADER, 0, &shader ) & ERROR ) { throw exception(); }
		Log( "Loaded frag shader" );
		shadermgr->addShaderParameter( shader, "in_Position", SHDIN_POSITION );
		shadermgr->addShaderParameter( shader, "in_Color", SHDIN_COLOR );
		shadermgr->addShaderParameter( shader, "in_Normal", SHDIN_NORMAL );
		shadermgr->addShaderParameter( shader, "in_Texcoord", SHDIN_TEXCOORD );
		shadermgr->addShaderParameter( shader, "lights", 0 );

		program->shaders.push_back( shader );
		shadermgr->linkProgram( program );
		renderer = shadermgr->createRenderer( program );
		Log(str(format( "Created shader (shade): %1%" ) % (ResourceManager::world->shadermgr->renderers.back()->program->programid) ));
		glUseProgram(0);



		// load highlight shader
		ShaderProgram *program2 = shadermgr->createProgram();
		glUseProgram( program2->programid );
		ShaderSet *shader2 = new ShaderSet();
		if ( shadermgr->loadShader( program2, "data/shaders/highlight.vert", GL_VERTEX_SHADER, SHD_HGHLT, &shader2 ) & ERROR ) { throw exception(); }
		Log( "Loaded highlight vert shader" );
		if ( shadermgr->loadShader( program2, "data/shaders/highlight.frag", GL_FRAGMENT_SHADER, 0, &shader2 ) & ERROR ) { throw exception(); }
		Log( "Loaded highlight frag shader" );
		shadermgr->addShaderParameter( shader2, "in_Position", SHDIN_POSITION );
		shadermgr->addShaderParameter( shader2, "in_Color", SHDIN_COLOR );
		shadermgr->addShaderParameter( shader2, "in_Normal", SHDIN_NORMAL );
		shadermgr->addShaderParameter( shader2, "in_Texcoord", SHDIN_TEXCOORD );

		program2->shaders.push_back( shader2 );
		shadermgr->linkProgram( program2 );
		RenderGroup* renderer2 = shadermgr->createRenderer( program2 );
		Log(str(format( "Created shader (highlight): %1%" ) % (ResourceManager::world->shadermgr->renderers.back()->program->programid) ));
		glUseProgram(0);



		// load UI shader
		ShaderProgram *program3 = shadermgr->createProgram();
		glUseProgram( program3->programid );
		ShaderSet *shader3 = new ShaderSet();
		if ( shadermgr->loadShader( program3, "data/shaders/ui.vert", GL_VERTEX_SHADER, SHD_NONE, &shader3 ) & ERROR ) { throw exception(); }
		Log( "Loaded UI vert shader" );
		if ( shadermgr->loadShader( program3, "data/shaders/ui.frag", GL_FRAGMENT_SHADER, 0, &shader3 ) & ERROR ) { throw exception(); }
		Log( "Loaded UI frag shader" );
		shadermgr->addShaderParameter( shader3, "in_Position", SHDIN_POSITION );
		shadermgr->addShaderParameter( shader3, "in_Texcoord", SHDIN_TEXCOORD );

		program3->shaders.push_back( shader3 );
		shadermgr->linkProgram( program3 );
		RenderGroup* renderer3 = shadermgr->createRenderer( program3 );
		Log(str(format( "Created shader (UI): %1%" ) % (ResourceManager::world->shadermgr->renderers.back()->program->programid) ));
		glUseProgram(0);

	}
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Add world objects
	ResourceManager::LoadMesh( renderer, "data/cube.obj" );
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


	if ( renderable ) {

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// Add lights

		GLint light;
		light = glGetUniformLocation( program->programid, "lights[0].position" );
		glUniform3fv( light, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.0f)) );
		light = glGetUniformLocation( program->programid, "lights[0].diffuse" );
		glUniform3fv( light, 1, glm::value_ptr(glm::vec3(0.0, 1.0, 0.425)) );

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


		glUseProgram( program->programid );
		glEnable( GL_TEXTURE_2D );
		glEnable( GL_CULL_FACE );
		glCullFace( GL_BACK );
		glFrontFace( GL_CCW );
		glProvokingVertex( GL_FIRST_VERTEX_CONVENTION );
		glEnable( GL_DEPTH_TEST );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_LEQUAL );
		glDepthRange( NGL_NEAR, NGL_FAR );
	}


	buildPages();
	selection = 0;
	page = 0;
	friction = 0.5f;
}
// ============================================== //


// ============================================== //
World::~World() {
	if (shadermgr) delete shadermgr;
	
	for (int y=0; y<npages_y; y++) {
		for (int x=0; x<npages_x; x++) {
			delete pages[y][x];
		}
		// delete[] pages[y];
	}
	// delete[] pages;
	for ( auto entity : entities ) {
		delete entity;
	}
	entities.clear();
}
// ============================================== //


// ============================================== //
void World::render() {
	if ( renderable ) {
		for ( auto renderer : shadermgr->renderers ) {
			glUseProgram( renderer->program->programid );
			for ( auto entity : renderer->entities ) {
				if ( inVisualSphere(entity) ) entity->mesh->render();
			}
		}
	}
}
// ============================================== //


// ============================================== //
// Cast a ray outwards from the camera (in the given x/y window coords)
// Returns the nearest Entity hit
Entity* World::worldPick(float xw, float yw) {

	glm::vec3 rayPos = camera.position;

	// ray direction
	glm::vec4 rayDir      = glm::vec4( xw, yw, 1.0f, 1.0f );
	glm::mat4 perspective = camera.perspective;
	glm::quat quatY       = glm::angleAxis( glm::degrees( camera.rotation.y ), glm::vec3( -1.0f, 0.0f, 0.0f ) );
	glm::quat quatX       = glm::angleAxis( glm::degrees( camera.rotation.x ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
	glm::quat quat        = quatY * quatX;
	glm::mat4 view        = glm::toMat4(quat);


	// check for the nearest Entity hit
	bool pickedSomething = false;
	Entity* newSelection = NULL;
	float nearestHit = 0.0f;
	float thisHit    = 0.0f;
	for( auto m : entities ) {
		glm::vec4 modelRayDir = glm::inverse(view) * glm::inverse(perspective) * rayDir;
		// glm::vec4 modelRayDir = glm::inverse(view) * rayDir;
		modelRayDir.x /= modelRayDir.w;
		modelRayDir.y /= modelRayDir.w;
		modelRayDir.z /= modelRayDir.w;
		modelRayDir.w /= modelRayDir.w;
		modelRayDir = glm::normalize(modelRayDir);
		Log(str(format("Trying to select.. (%1%,%2%,%3%) + t<%4%,%5%,%6%>")%rayPos.x%rayPos.y%rayPos.z%modelRayDir.x%modelRayDir.y%modelRayDir.z));
		if ( ( thisHit = m->mesh->lineIntersects( rayPos, modelRayDir.xyz ) ) > 0.0f ) {
			// pick this object
			if ( selection && m == selection ) Log("  Already picked this Entity!");
			else Log("  Picked Entity!");

			// is this hit closer than the other hit?
			Log(str(format("  Picked at: (%1%)")%thisHit));
			if ( newSelection ) {
				if ( thisHit < nearestHit ) {
					newSelection = m;
					nearestHit = thisHit;
				}
				continue;
			} else {
				newSelection = m;
				nearestHit = thisHit;
			}
			pickedSomething = true;
		}
	}

	if ( selection == newSelection ) return selection;


	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	/*
	if ( selection ) {
		// unselect entity
		bool done = false;
		selection->selected = false;
		for ( auto renderer : shadermgr->renderers ) {
			if (done) break;
			int i=0;
			for ( auto entity : renderer->entities ) {
				if ( entity == selection ) {
					Log("unselecting entity..");
					entity->mesh->renderable = false;
					renderer->entities.erase(renderer->entities.begin()+i);
					shadermgr->renderers.front()->entities.push_back(entity);
					entity->mesh->renderData->gl = shadermgr->renderers.front()->program->programid;
					entity->mesh->renderData->clear();
					entity->mesh->renderData->construct(entity->mesh,true);
					done = true;
					entity->mesh->renderable = true;
					break;
				}
				i++;
			}
		}
		selection = NULL;

	}

	if ( newSelection ) {
		// select entity
		bool done = false;
		for ( auto renderer : shadermgr->renderers ) {
			if (done) break;
			int i=0;
			for ( auto entity : renderer->entities ) {
				if ( entity == newSelection ) {
					Log("selecting entity..");
					entity->mesh->renderable = false;
					renderer->entities.erase(renderer->entities.begin()+i);
					shadermgr->renderers.back()->entities.push_back(entity);
					entity->mesh->renderData->gl = shadermgr->renderers.back()->program->programid;
					entity->mesh->renderData->clear();
					entity->mesh->renderData->construct(entity->mesh,true);
					selection = entity;
					entity->selected = true;
					done = true;
					entity->mesh->renderable = true;
					Log("selected entity..");
					break;
				}
				i++;
			}
		}

	}
	*/
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


	return newSelection;
}
// ============================================== //


// ============================================== //
void World::step(float ms, WorldActionResponses* responses) {

	// Apply all awaiting actions (received from input or network)
	actions.swap();
	// TODO: using WorldAction& pushes this into an infinite loop, loading the same action over and over again ??
	for ( WorldAction action : (**actions.inactive) ) {
		// apply action
		WorldActionResponse* response = action.apply( this, false );
		if ( responses ) {
			responses->push_back( response );
		}
	}
	(*actions.inactive)->clear();


	// Step through each Entity (eg. velocity)
	for ( auto entity : entities ) {
		if ( fabs(entity->velocity.x) + fabs(entity->velocity.y) + fabs(entity->velocity.z) < 1.0f ) continue;
		Log(str(format("Velocity (%1%)")%entity->guid));
		entity->mesh->position += ms / 1000 * entity->velocity;
		entity->velocity *= friction;
		updateEntityPage(entity);
		Log(str(format("NEW POSITION: <%1%,%2%,%3%>")%entity->mesh->position.x%entity->mesh->position.y%entity->mesh->position.z));

		// collision detection!
		bool collided = false;
		for ( auto collidable : entities ) {
			if ( collidable == entity ) continue;
			if ( entity->collides( collidable ) ) {
				Log(str(format("Collision! (%1%: <%2%,%3%,%4%> HITS %5%: <%6%,%7%,%8%>")%entity->guid%entity->mesh->position.x%entity->mesh->position.y%entity->mesh->position.z%collidable->guid%collidable->mesh->position.x%collidable->mesh->position.y%collidable->mesh->position.z));
				// Collides!
				collidable->velocity += entity->velocity;
				collided = true;

				// push this collided object forward to avoid a hook-lock
				while ( entity->collides( collidable ) ) {
					Log(".......cheating a little to avoid hook-lock");
					collidable->mesh->position += ms / 1000 * collidable->velocity;
				}
				updateEntityPage(collidable);
			}
		}

		if ( collided ) entity->velocity = glm::vec3( 0.0f, 0.0f, 0.0f );
	}
}
// ============================================== //


// ============================================== //
void World::pushAction(int action, Bfield128_t args) {
	// TODO: test if action is legal
	(*actions.active)->push_back( WorldAction(action, args) );
}
// ============================================== //


// ============================================== //
void World::buildPages() {
	npages_y = WRLD_MAX_HEIGHT/WRLD_PAGE_HEIGHT;
	npages_x = WRLD_MAX_WIDTH/WRLD_PAGE_WIDTH;

	for (int y=0; y<npages_y; y++) {
		for (int x=0; x<npages_x; x++) {
			pages[y][x] = new Page();
			pages[y][x]->center = glm::vec2(
					y*WRLD_PAGE_HEIGHT - (npages_y*WRLD_PAGE_HEIGHT)/2,
					x*WRLD_PAGE_WIDTH  - (npages_x*WRLD_PAGE_WIDTH)/2 );
		}
	}

	for (int y=0; y<npages_y; y++) {
		for (int x=0; x<npages_x; x++) {
			buildPage(pages[y][x]);
		}
	}

	for ( auto entity : entities ) {
		fetchPage(entity)->addEntity(entity);
	}
	page = fetchPage(camera.position.z, camera.position.x);
}
// ============================================== //


// ============================================== //
void World::buildPage(Page* page) {
	// set the proper neighbouring pages
	glm::vec2 center = page->center;
	int y, x;
	Page* neighbour = NULL;
	if ( page->n == NULL ) {
		y = fetchPageIndexY( center.y + WRLD_PAGE_HEIGHT );
		x = fetchPageIndexX( center.x );
		neighbour = fetchPage(y,x);
		page->n = neighbour;
	} if ( page->ne == NULL ) {
		y = fetchPageIndexY( center.y + WRLD_PAGE_HEIGHT );
		x = fetchPageIndexX( center.x + WRLD_PAGE_WIDTH );
		neighbour = fetchPage(y,x);
		page->ne = neighbour;
	} if ( page->e == NULL ) {
		y = fetchPageIndexY( center.y );
		x = fetchPageIndexX( center.x + WRLD_PAGE_WIDTH );
		neighbour = fetchPage(y,x);
		page->e = neighbour;
	} if ( page->se == NULL ) {
		y = fetchPageIndexY( center.y - WRLD_PAGE_HEIGHT );
		x = fetchPageIndexX( center.x + WRLD_PAGE_WIDTH );
		neighbour = fetchPage(y,x);
		page->se = neighbour;
	} if ( page->s == NULL ) {
		y = fetchPageIndexY( center.y - WRLD_PAGE_HEIGHT );
		x = fetchPageIndexX( center.x );
		neighbour = fetchPage(y,x);
		page->s = neighbour;
	} if ( page->sw == NULL ) {
		y = fetchPageIndexY( center.y - WRLD_PAGE_HEIGHT );
		x = fetchPageIndexX( center.x - WRLD_PAGE_WIDTH );
		neighbour = fetchPage(y,x);
		page->sw = neighbour;
	} if ( page->w == NULL ) {
		y = fetchPageIndexY( center.y );
		x = fetchPageIndexX( center.x - WRLD_PAGE_WIDTH );
		neighbour = fetchPage(y,x);
		page->w = neighbour;
	} if ( page->nw == NULL ) {
		y = fetchPageIndexY( center.y + WRLD_PAGE_HEIGHT );
		x = fetchPageIndexX( center.x - WRLD_PAGE_WIDTH );
		neighbour = fetchPage(y,x);
		page->nw = neighbour;
	}

}
// ============================================== //


// ============================================== //
int World::fetchPageIndexX(int x) {
	int index = x/WRLD_PAGE_WIDTH + npages_x/2;
	if ( index < 0 ) index = 0;
	return ( index >= npages_x-1 ? npages_x : index );
}
// ============================================== //


// ============================================== //
int World::fetchPageIndexY(int y) {
	int index = y/WRLD_PAGE_HEIGHT + npages_y/2;
	if ( index < 0 ) index = 0;
	return ( index >= npages_y-1 ? npages_y : index );
}
// ============================================== //


// ============================================== //
Page* World::fetchPage(int y, int x) {
	return pages[fetchPageIndexY(y)][fetchPageIndexX(x)];
}
// ============================================== //


// ============================================== //
Page* World::fetchPage(Entity* entity) {
	return fetchPage(entity->mesh->position.z, entity->mesh->position.x);
}
// ============================================== //


// ============================================== //
void World::updateEntityPage(Entity* entity) {
	if ( fetchPage(entity) != entity->page ) {
		if ( entity->page ) entity->page->removeEntity(entity);
		fetchPage(entity)->addEntity(entity);
	}
}
// ============================================== //


// ============================================== //
bool World::inVisualSphere(Entity* entity) {
	return ( fabs(entity->mesh->position.y ) - fabs(camera.position.y) <= CAM_FAR &&
			 fabs(entity->mesh->position.x ) - fabs(camera.position.x) <= CAM_FAR );
}
// ============================================== //


// ============================================== //
void Page::addEntity(Entity* entity) {
	entities.push_back(entity);
	entity->page = this;
}
// ============================================== //


// ============================================== //
void Page::removeEntity(Entity* entity) {
	int i=0;
	entity->page = 0;
	for ( auto ent : entities ) {
		if ( ent == entity ) {
			entities.erase(entities.begin()+i);
			return;
		}
		i++;
	}
}
// ============================================== //


WorldActionResponse::WorldActionResponse() : worldAction(0), succeeded(false) { }
WorldActionResponse::WorldActionResponse(const WorldAction& worldAction, bool succeeded) : succeeded(succeeded) {
	this->worldAction = new WorldAction(worldAction);
}
WorldActionResponse::WorldActionResponse(const WorldActionResponse& rhs) {
	this->succeeded = rhs.succeeded;
	this->worldAction = new WorldAction(*rhs.worldAction);
}
WorldActionResponse& WorldActionResponse::operator=(const WorldActionResponse& rhs) {
	this->succeeded = rhs.succeeded;
	this->worldAction = new WorldAction(*rhs.worldAction);
	return *this;
}
WorldActionResponse::~WorldActionResponse() {
	delete this->worldAction;
}


WorldAction::WorldAction(uint action, const Bfield128_t& args, int page, int initiator, uint id) : action(action), page(page), initiator(initiator), id(id) { this->args.copy(args); } 
WorldAction::WorldAction(const WorldAction &rhs) { this->action = rhs.action;
	this->args = rhs.args;
	this->page = rhs.page; this->initiator = rhs.initiator; this->id = rhs.id; }
WorldAction& WorldAction::operator=(const WorldAction &rhs) { this->action = rhs.action;
	this->args = rhs.args;
	this->page = rhs.page; this->initiator = rhs.initiator; this->id = rhs.id;  return *this; }
WorldAction::~WorldAction() {  }



// ============================================== //
// Drop a mesh entity into the world
WorldAction WorldAction::serialize_create_mesh(unsigned int mesh, unsigned int guid, glm::vec3 position) {
	uint action; Bfield128_t args;
	action = (ACTION_CREATE);

	unsigned char offset = 0;
	args.set<uchar>(ACTION_ARGS_CREATE_MESH, offset); offset += sizeof(ACTION_ARGS_CREATE_MESH);
	args.set<uint>(mesh, offset); offset += sizeof(mesh);
	args.set<uint>(guid, offset); offset += sizeof(guid);

	glm::ivec3 posOffset;
	posOffset.x = (int)position.x;
	posOffset.y = (int)position.y;
	posOffset.z = (int)position.z;
	args.set<int>( quantizeFloat( position.x - posOffset.x ), offset ); offset += sizeof(int);
	args.set<int>( quantizeFloat( position.y - posOffset.y ), offset ); offset += sizeof(int);
	args.set<int>( quantizeFloat( position.z - posOffset.z ), offset ); offset += sizeof(int);
	args.set<int>( posOffset.x, offset ); offset += sizeof(int);
	args.set<int>( posOffset.y, offset ); offset += sizeof(int);
	args.set<int>( posOffset.z, offset ); offset += sizeof(int);
	Log("Serializing WorldAction..");
	return WorldAction( action, args );
}
// ============================================== //




// ============================================== //
// Drop a light entity into the world
WorldAction WorldAction::serialize_create_light(unsigned int guid, glm::vec3 color, glm::vec3 position) {
	// TODO
	Bfield128_t args;
	return WorldAction( 0, args );
}
// ============================================== //


// ============================================== //
// Add some velocity into an entity
WorldAction WorldAction::serialize_push_entity(unsigned int guid, glm::vec3 direction, uchar speed) {
	uint action; Bfield128_t args;
	action = (ACTION_PUSH);

	unsigned char offset = 0;
	args.set<uint>(guid, offset); offset += sizeof(guid);

	args.set<int>( quantizeFloat( direction.x ), offset ); offset += sizeof(int);
	args.set<int>( quantizeFloat( direction.y ), offset ); offset += sizeof(int);
	args.set<int>( quantizeFloat( direction.z ), offset ); offset += sizeof(int);
	args.set<uchar>( speed, offset ); offset += sizeof(int);

	return WorldAction( action, args );

}
// ============================================== //


// ============================================== //
WorldActionResponse* WorldAction::apply(World* world, bool peekOutcome) {
	switch ( action ) {
		case ACTION_CREATE:
			switch ( args.fetch<uchar>(0) ) {
				case ACTION_ARGS_CREATE_MESH:
					return apply_create_mesh(world, peekOutcome); break;
				case ACTION_ARGS_CREATE_LIGHT:
					return apply_create_light(world, peekOutcome); break;
			}
			break;
		case ACTION_PUSH:
			return apply_push_entity(world, peekOutcome);
			break;
	}
	return new WorldActionResponse();
}
// ============================================== //


// ============================================== //
WorldActionResponse* WorldAction::apply_create_mesh(World* world, bool peekOutcome) {
	uint mesh, guid;
	glm::ivec3 posOffset;
	glm::vec3 position;
	unsigned char offset = sizeof(uchar);

	mesh = args.fetch<uint>(offset); offset += sizeof(mesh);
	guid = args.fetch<uint>(offset); offset += sizeof(mesh);
	position.x = dequantize( args.fetch<int>(offset) ); offset += sizeof(int);
	position.y = dequantize( args.fetch<int>(offset) ); offset += sizeof(int);
	position.z = dequantize( args.fetch<int>(offset) ); offset += sizeof(int);
	posOffset.x = args.fetch<int>(offset); offset += sizeof(int);
	posOffset.y = args.fetch<int>(offset); offset += sizeof(int);
	posOffset.z = args.fetch<int>(offset); offset += sizeof(int);

	position.x += posOffset.x;
	position.y += posOffset.y;
	position.z += posOffset.z;


	RenderGroup* renderer = 0;
	if ( world->renderable ) {
		renderer = world->shadermgr->renderers.front();
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				Entity* drop = ResourceManager::LoadMesh( renderer , "data/cube.obj" );
				drop->guid = guid;
				Mesh* dropMesh = drop->mesh;
				dropMesh->position = position;
				dropMesh->position.z *= -1;
				dropMesh->position.x *= -1;
				dropMesh->position.y *= -1;
				world->updateEntityPage(drop);
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	return new WorldActionResponse( WorldAction( action, args, page, initiator, id ), true );
}
// ============================================== //


// ============================================== //
WorldActionResponse* WorldAction::apply_create_light(World* world, bool peekOutcome) {
	return new WorldActionResponse( WorldAction( action, args, page, initiator, id ), true );
}
// ============================================== //


// ============================================== //
WorldActionResponse* WorldAction::apply_push_entity(World* world, bool peekOutcome) {

	uint guid;
	glm::vec3 direction;
	uchar speed;
	unsigned char offset = 0;

	guid = args.fetch<uint>(offset); offset += sizeof(guid);
	direction.x = dequantize( args.fetch<int>(offset) ); offset += sizeof(int);
	direction.y = dequantize( args.fetch<int>(offset) ); offset += sizeof(int);
	direction.z = dequantize( args.fetch<int>(offset) ); offset += sizeof(int);
	speed = args.fetch<uchar>(offset); offset += sizeof(uchar);

					Entity* selection = 0;
					for ( Entity* ent : world->entities ) {
						if ( ent->guid == guid ) {
							selection = ent;
							break;
						}
					}
					if ( selection == 0 ) {
						return new WorldActionResponse();
					}
					glm::vec4 push = glm::vec4( direction.x, direction.y, direction.z, 1.0f ) * speed;
					Log(str(format("Pushing (%1%) with: <%2%,%3%,%4%>")%guid%push.x%push.y%push.z));
					selection->velocity += glm::vec3( push.x, push.y, push.z );

	return new WorldActionResponse( WorldAction( action, args, page, initiator, id ), true );
}
// ============================================== //


