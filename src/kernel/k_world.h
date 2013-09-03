#ifndef __K_WORLD_H__
#define __K_WORLD_H__


#include "util.inc.h"
#include "libutils/lib_resmgr.h"
#include "libutils/lib_shdmgr.h"
#include "kernel/k_camera.h"

/*
 * World
 *
 * TODO
 *
 *  > server/client world
 *
 ***/


/*
=================================================

	World

	Handles a single map zone; contains all entities and zone
	specific details

=================================================
*/

struct WorldAction;
// typedef union { WorldAction* worldAction; bool succeeded; } WorldActionResponse;
struct WorldActionResponse {
	WorldActionResponse();
	WorldActionResponse(const WorldAction& worldAction, bool succeeded);
	WorldActionResponse(const WorldActionResponse& rhs);
	WorldActionResponse& operator=(const WorldActionResponse& rhs);
	~WorldActionResponse();
	WorldAction* worldAction;
	bool succeeded;
};
template<class T> struct SwapBuffer;
typedef SwapBuffer<vector<WorldAction>> WorldActions;
typedef vector<WorldActionResponse*> WorldActionResponses;
class ShaderManager;
class Entity;
struct Page;
class World {
	float friction;
	Entity* selection;
public:
	ShaderManager* shadermgr;
	World(bool renderable);
	~World();

	void loadWorld();
	void render();
	Entity* worldPick(float xw, float yw);
	void step(float ms, WorldActionResponses* responses = 0); // step physics forward

	vector<Entity*> entities;
	int npages_x, npages_y;
	Page* pages[(int)(WRLD_MAX_HEIGHT/WRLD_PAGE_HEIGHT)][(int)(WRLD_MAX_WIDTH/WRLD_PAGE_WIDTH)];
	Page* page;
	void buildPages();
	void buildPage(Page*);
	Page* fetchPage(Entity*);
	Page* fetchPage(int,int);
	int fetchPageIndexY(int);
	int fetchPageIndexX(int);
	void updateEntityPage(Entity*);
	static bool inVisualSphere(Entity*);


	WorldActions actions;
	void pushAction(int actionType, Bfield128_t args);

	bool renderable;
};

struct Page {
	Page() {
		n=ne=e=se=s=sw=w=nw=NULL;
	}
	vector<Entity*> entities;

	void addEntity(Entity*);
	void removeEntity(Entity*);
	glm::vec2 center;
	Page* n ;
	Page* ne;
	Page* e ;
	Page* se;
	Page* s ;
	Page* sw;
	Page* w ;
	Page* nw;
};

static const uint ACTION_CREATE = 1;
static const uint ACTION_PUSH   = 2;

static const uchar ACTION_ARGS_CREATE_MESH  = 0;
static const uchar ACTION_ARGS_CREATE_LIGHT = 1;


struct WorldAction {
	WorldAction(uint action, const Bfield128_t& args, int page = -1, int initiator = -1, uint id = 0);
	WorldAction(const WorldAction &rhs);
	WorldAction& operator=(const WorldAction &rhs);
	~WorldAction();

	int page; int initiator; uint id;
	uint action;
	Bfield128_t args;

	/* Serialize Actions
	 *
	 * Actions can be created locally, stored, streamed and
	 * serialized to be passed around as action objects.
	 * The following list of serialization functions will
	 * be responsible for creating an action which can be
	 * read elsewhere
	 ***/
	static WorldAction serialize_create_mesh(unsigned int mesh, unsigned int guid, glm::vec3 position); // create an entity based off of a mesh
	static WorldAction serialize_create_light(unsigned int guid, glm::vec3 color, glm::vec3 position); // create a light
	static WorldAction serialize_push_entity(unsigned int guid, glm::vec3 direction, uchar speed); // add velocity to a given entity

	/* Apply Actions
	 *
	 * Actions can be applied to the world through; all
	 * action decoding from the already serialized action
	 * will be done here
	 *
	 * In the case that the action is impossible, a
	 * WorldActionResponse object is returned with a
	 * modified version of the WorldAction in such a way
	 * that the outcome is close the desired outcome, but
	 * actually possible to do. In case the apply is
	 * successful, WorldActionResponse has worldAction set
	 * to null, and succeeded set to true
	 ***/
	WorldActionResponse* apply(World* world, bool peekOutcome = false);

private:
	WorldActionResponse* apply_create_mesh(World* world, bool peekOutcome);
	WorldActionResponse* apply_create_light(World* world, bool peekOutcome);
	WorldActionResponse* apply_push_entity(World* world, bool peekOutcome);
};


#endif

