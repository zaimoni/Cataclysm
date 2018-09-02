#ifndef _MONDEATH_H_
#define _MONDEATH_H_

class game;
class monster;

class mdeath
{
public:
 static void normal		(game *g, monster *z); // Drop a body
 static void acid		(game *g, monster *z); // Acid instead of a body
 static void boomer		(game *g, monster *z); // Explodes in vomit :3
 static void kill_vines	(game *g, monster *z); // Kill all nearby vines
 static void vine_cut		(game *g, monster *z); // Kill adjacent vine if it's cut
 static void triffid_heart	(game *g, monster *z); // Destroy all roots
 static void fungus		(game *g, monster *z); // Explodes in spores D:
 static void fungusawake	(game *g, monster *z); // Turn into live fungaloid
 static void disintegrate	(game *g, monster *z); // Falls apart
 static void shriek		(game *g, monster *z); // Screams loudly
 static void worm		(game *g, monster *z); // Spawns 2 half-worms
 static void disappear		(game *g, monster *z); // Hallucination disappears
 static void guilt		(game *g, monster *z); // Morale penalty
 static void blobsplit		(game *g, monster *z); // Creates more blobs
 static void melt		(game *g, monster *z); // Normal death, but melts
 static void amigara		(game *g, monster *z); // Removes hypnosis if last one
 static void thing		(game *g, monster *z); // Turn into a full thing
 static void explode		(game *g, monster *z); // Damaging explosion
 static void ratking		(game *g, monster *z); // Cure verminitis
 static void kill_breathers	(game *g, monster *z); // All breathers die

 static void gameover		(game *g, monster *z); // Game over!  Defense mode
};

#endif
