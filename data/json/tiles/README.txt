This directory contains JSON configuration files for tilesets.

Active configurations are listed as relative file paths in a JSON array: data/json/tilespec.json 
A tileset configuration is a JSON object with two keys:
* name: value is the display name of the tileset, for use in a vaporware configuration utility.
* tiles: value is a JSON object.
* The values of this JSON object are unique identifiers of the relevant tiles.  Syntax is a string
	relative-file-path[#index][!transform-spec]
**	relative filepath is required, and must name an image file.  If that image file is a tilesheet, the dimension of the square tiles
	within the tilesheet must be a numeric infix.
**	#index is required when targeting a tile within a tilesheet (otherwise the image is considered a single tile).
**	the vaporware !transform-spec specifies a white-listed image transform, including vaporware rotation and vaporware image-flip options.
**	The JSON-reserved literal null may be used to explicitly unset a tile assignment.
* The keys of this JSON object designate, in a vaporware way, entities which are to have tiles assigned.

Tileset loading happens *very* early (it influences font size)
