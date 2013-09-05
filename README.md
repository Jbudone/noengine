
No Engine
===============


JB's engine collection. Until the engine and core is in a working and usable state, this will be
known as No Engine (ie. incomplete) The various engines will be completely scalable with the
possibility of multiple implementations which can be choosen at compile-time.

Requirements
----------

	+ freeglut3-dev
	+ boost 1.5.3+



NOTES
----------
	

	+ Mesh/Entity handling

		Resource manager will load, store and manage all meshes and their data (vertices, textures,
		animations, sounds, scripts, metadata); able to load each module independently, then link
		together in various entity types (entity, actor, static, movable, etc.). Individual modules
		are stored in different locations, where the entity types can use each module to point to
		them. World will request instance of entity, resource manager can create and store instance
		locally, but return a reference to the entity where World can work with the reference
	
	+ Map handling

		'''OPTION 1'''
		Maps are loaded from map files; they store a variety of entity objects (each entity is a
		derived base object -- eg. light, actor, static obj) where each base object has its own pool
		facility. Entities are described by brushes in the map; type of object (Light: point,
		spotlight, directional light, etc.), deriving object parameters, colliding physics (k-top,
		square, sphere, etc.), scripts, any extra details. Entity should contain bare basics, while
		base objects should contain as much data while being totally generic; Derived object will
		only override virtual methods from Base obj (allows for safe storage capacity in pools for
		base objects), but base objects can contain a pointer to an extra data or parameter type
		object in which the derived object can use for extra data
		Entity -> Base Object -> Derived Object

		'''OPTION 2'''
		Go through file and quickly count how many individual derived objects exist within each
		derived class; create an array in the resource manager to store these derived objects. Map
		object will instance those derived objects, with individual instance data and a pointer to
		the base object in resource mgr
		NOTE: may not need to count first; if all brushes are parsed first then we can continuously
				move allocations into bigger pools until we're finished reading them (since nothing
				will point to those brushes just yet anyways)

		QS: how to load multiple primitives for mesh objects
		QS: how to store instances in map obj? (different parameters necessary for different derived
				objects; instance will be given extra data within map file, and that data can change
				throughout the gameplay)
		QS: how to handle paging (best way to store objects to allow for scalable paging; including
				polysoups like heightmaps; need to reference instancing count & geometry shader data
				which should scale along the paging)




TODO
---------

	+ Resource Mgr: instance meshes & textures; test UV-baked textures; allow tex arrays (normal
			maps, spec maps); tex & mesh table lookup + string hashing; load all wavefront file
			details
	+ Multiple worlds; held outside of res
	+ Setup Mesh kernel: load wavefront files (ALL data + material files)
	+ Setup UI
	+ Setup testing env: no threads, single player
	+ Setup Job queue for threads
	+ Dev/Release compiling
	+ look into Erlang/Haskell messaging system between objects
	+ Hash id (low collision & fast): world actions
	+ Error handling: GL include line numbers & details of GL commands being issued
	
	+ Serializer/Parser for saving/loading maps, clients initially connecting or loading pages
	+ Voxel based terrain engine w/ sculpting brushes
	+ Add items to world, rotate/scale/move, edit shaders/materials & live serializing/reloading
		object collision detection (rotation, scaling; level of detail in collision: k-top)
	+ Automatic portals through voxel terrain
	+ Terrain collision detection
	+ Skyboxes w/ weather system, day-night system, calendar/seasonal system
	+ Mipmapping in pages; network sends through pages/portals; low priority job queue to update
			further away pages; favour portals over pages
	+ Save/Load/Snapshot/Pause state for World, Entity, Network, etc. (easily reload snapshots
			for later testing; automated snapshots from client/server regularly)
	+ Load Collada files
	+ Journaling system
	+ Scripting language for AI, triggerable objects, actors, etc.
	+ Select/view items state based machine archistructure
	+ Tessellation shading
	+ Count CPU performance (FPS rate), and manage shader quality accordingly
	+ Dynamic lighting
	+ Multipass rendering
	+ Object query system
	+ High precision clock
	+ Action buffer resyncing
	+ Server journaling; client ack on relays, able to resend previous relays. Journaler handled by
		a separate thread for performance


Cleanup
--------
	
	+ Networking component
	+ Clean logging/comments
	+ Setup & enforce code standards
	+ Variable naming (more readable)



Game Thoughts
--------------

	+ Begin Gameplay design (religions - dying in pvp battle makes you become their friend [daoc
			style] ?)
		> NPC's have personalities (certain characteristic levels -- honour, compassion, distrust,
				leadership, etc.), goals (weighed), 
		> Groups of NPC's also have goals and personality styles (based off the main mentality of
				the group); Group's of Group's (a group of NPC's -- friends; group lives within a
				certain faction -- faction is apart of a certain kingdome); Friends, relationships
				Kingdoms may hold events to keep people happy (tournaments w/ gambling -- limited
					number of gambling currency around, have to buy through us off of random goblin
					group or something; jesters; hangings; plays..users could be paid in-game to
					hold plays, etc; mages showing off -- GM players could be given global mic access
					Group's have leadership roles and adaptive ranking systems
				Schedules work by a state-based machine which goes from 1 item to the next w/
				various influences acting as input (eg. time, external events, external influences
					like beer or meeting a girl etc.)
				Have multiple languages which also need to be learned
				How do I (npc) help the outside world: I want whats best for the kingdom, and the
								kingdom needs these items currently (prioritized), this is how each
								of which is ranked in toughness and how much I care to equal out
								whats best for me to handle, do I have the requrirements to solve
								this, yes? then give me the step-by-steps to go through in order to
									-- NOTE: extend current requirements/abilities/characteristics
										as much as possible in order to make it extremely realistic
										(able to steal from old lady or street thug; 
								solve this problem for the kingdom ;; what about my other life
								concerns and how much I want to
								hang-out/mate/be-mean/help-kingdom/learn-new-abilities/practice-
								skills (athletes, flirting, ), take care of somebody/something, go
								to fun events for fun (bar, tournaments-sports fights ctf  , hangings, plays, 
									Able to gain strength from CTF games AND make money, going out and
									fighting is much more dangerous & tougher plus you might die and lose
									EVERYTHING whereas you could just live in a kingdom and get by
									and join the culture and cultivation of the kingdom // What
									about kingdom wars? ..slavery? jail break from kingdom? saving
									old friends? saving the king? 
								Going out intot he world you could explore goblin nests and 
					Events that happen are recorded by people into a database of an event that
					happened (in an archive which you are able to access and remember), 
					You access memories before making decisions on major events that are going on

					There are a continous flow of inputs of various types coming at you but you can
					ignore some of them based on their ranking level in various characteristics of
					what that event/input are and your comparison of caring about that particular
					thing
				Leadership people want to find something GREAT (they agree in and believe could be
					the best for the world according to what they believe -- this also is influenced
					by what the player expects the world is currently like and what other kingdoms
					may be like...this could also be influenced by what other NPC's say via. rumors
					and stories and your belief level for that person based on their charm & how
					they look and are appreciated by other users (note some users may want to join
						in the community to be appreciated by other NPC's in order to raise their
						self-appreciation level INCLUDING joining in hangings and beatings)
					to join and raise themselves to the higher ranked part of this great kingdom (
						some might be a low leadership level for just becoming a trademan at the
						kingdom), otherwise start their own if they think they've explored enough
					and cannot find their own (explored enough is based on how curious they are, how
						scared they are by various monsters they run into, how much they've explored
						already and want/need to settle down)
				AI cares about various items/people/objects/events (physical or metaphorical?
					object), and has motivation to keep it, can get upset about losing it, create a
					quest for getting it back?
		> Trades -- jewelry could only be enhanced by a very special individual whom must bring the
					item over to a place where an important event takes place
					buildings require wood, minerals, ore, etc.; alchemy for potions, etc.
					NO market house, however NPC merchants will sometimes haggle for your items and
					charge it accordingly w/ kingdom tax included
					Items are SHOCKINGLY rare; extremely cheap like in early Everquest, then make
					mining/smithing/enchanting more useful BUT make them really tough and tedious to
					keep up with and to do a proper zap to make a sellable item (earlier ones are
					trashy ones); potions/food/scroll help with temporary buffs/utilities too (perma
					ones too for REALLY rare items like dragon meat) -- some of which also helps
					your learning ability for trades and such
		> Death -- lose importancy in factions, forgotten history? (losing a bit of yourself,
				quests you've done, favours for NPC's, etc.), lose ALL non-soul bound items, each
				map could contain local postal office things (to mail items back to your bank over
				time), those things cost $$ to use w/ tax from the kingdom that bought it, they may
				hire more guards to protect the area in case random groups come to rob the map bank)
				ghosts go to local haunted area in which they can attack people trying to rob the
				haunted place, are next to invincible, and should be feared by everybody-- includes
				NPC ghosts too, need to attack a victim in order to come back to life? or do
				something horrible which would not be desirable by most people; could also go to
				hell at some levels/religions and have to find somebody within the entire waiting
				area of hell, competing with many other souls; these quests help to build hell's
				army); NPC's can become posessed and no longer able to talk, must do something
				wicked as a given goal in order to leave death state, but going ANY way off path
				will unposess the NPC and force user to go to another body

				..not being able to play for the rest of the day? then waking up in a bad place that
				you have to get out of (based on religion) -- possesed, hell, ghoul, zombie
				outbreak, ghost in haunted house, some random event demon outbreak, live vegetation
				growth which could otherwise be used for food (but you have to kill the player/npc
						instead to get some of your stat drops and exhaustion back), 

				A ghost is created and comes back to haunt you later (all their buffs, negatives, items, etc.)
				can also attack other people, haunt places & the fight scene

				major stat drop; note stats affect various things (str for carrying & pulling items/people; wis
				for resists; agi for dodging; etc..)

				ranking drop; ranking has certain tiers which once you pass you get way better overall. Dying
				drops you back to the beginning of the tier (OR the 1st tier level)
				NOTE: higher level tiers of people are HUGELY powerful, but should really fear death (to lose 
				their power)

				You revive at your soulbound spot (a kingdome or some owned area) after X time; this
				place gets an automated tax from all gold you earn, and a portion of what you lose
				upon dying
	> Combat -- able to climb and take hold of parts of enemy (run ontop and flip over while
				pulling their arm or something); able to interact w/ enemy body; able to push around
				enemy body (quaternion rotations); world objects interact with people & people with
				it; able to attack city walls and break it into pieces to be used
				Make the combat more arena like (q3) w/ fast paced action
				CTF games are major events 
				Various domination style runes which guilds can go after (outside of kingdom games)
					which are highly sought after because they give the guild/kingdom(kingdom main
							guild) overall characteristic boosts
	> Magic -- the longer you spend casting a spell, the more it charges up; this can also
	charge into multiple levels of the spell and add new characteristics (AOE, damage
			elements, ), you can either auto-charge or do a mini-game type thing (waving the
				wand in a 3d type follow the line game; make the game VERY detailed and wavey and
				beautiful looking, but its almost impossible to even get THAT accurate so its still
				somewhat forgiving; the smoke and magic particles are flowing so much through the
				screen that you notice a very subtle but sweet looking 3d line which you can follow
				in an orchestrating manner

				the people dolls that you stab and somehow magically affects them -- you can do that
				at certain times, but you must be fragile with the doll because it breaks really
				easily and is very rare/expensive to come by (very specific on whom you want)
		> Levelling -- level up characteristics (by a charge which takes exponentially longer to
				charge, and can be dropped lower by various influences).. Characteristics consist
				of stats(str/dex/int/wis/cha/spd/agl/con) religion
				factors(light/dark/earth/fire/water/ice/air) each of which can have an effect on
				your various skills you attempt to use. Say you level up fighting and you gain
				access to weapon specific upgrades (including hand-to-hand) [eg. taking hold of
				someone and flipping them over, chop/hack/etc., ] you can 
		> Designers -- sculpting item for magically sculpting terrain; gardening tools; pitchfork
				for objects (also rotate + scale + edit shader); all entities (including terrain)
				have an edit button for shaders + materials (also live reloading + serializing
				change globally, also AI characteristics & stats & inventory

Coding Style
--------------

	* Header files: marginwrap 50
	* Source files: marginwrap 80

	/***
	 * File
	 *
	 *	    Desc of how it works
	 *
	 * TODO
	 *  - x1
	 *  - x2
	 *			-details-
	 **/


	#include "rendering.inc.h"
	#include "util.inc.h"
	
	#include </usr/extern/xxx.h>
	#include "extern/GL/xxx.h"
	
	#include "local.h"


	Object {
	public:
		memberAbc;


		// group of functions
		// ---------------------
		// short description of this particular
		// group of functions
		// ---------------------
		func1(int x, int y, int z); // read this pixel
		func2(int x, int y, int z); // read that pixel
		func3(int x, int y, int z); // read those pixels

		// ---------------------
		// thisFunction
		//
		// An extended, long explanation of this function
		// ---------------------
	};

	// ============================================================================
	// Function blah
	//
	//  description (optional)
	// @param1: desc
	// @param2: desc
	// @returns: desc
	// ========================================================
	longFunction ( param1, param2, ... ) {

	}
	// ========================================================

	// ========================================================
	// Function foo
	// small description
	smallFunction ( param1, param2, ... ) {

	}
	// ========================================================


	/******************************************************************************/
								/***** SECTION *****/
	/******************************************************************************/
	/*

				Notes on the inner workings of this section (header)

																				*/
