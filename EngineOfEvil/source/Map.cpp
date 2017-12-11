#include "Game.h"

//**************
// eMap::Init
//**************
bool eMap::Init () {
	return LoadMap("Graphics/Maps/EvilTown.emap");
}

//**************
// eMap::LoadMap
// Populates tileMap's matrix for future collision and redraw
// using a file
// DEBUG (.emap file format):
// # first line comment\n
// # any number of leading comments with '#'\n
// Num_Columns: numColumns\n
// Num_Rows: numRows\n
// Cell_Width: cellWidth\n
// Cell_Height: cellHeight\n
// Num_Layers: numLayers\n
// Tileset_Filename: tileSetFilename.etls\n
// Layers {\n
// layer_1_name {\n
// master-tileSet-index, master-tileSet-index, ... master-tileSet-index\n
// }\n		(signifies end of a layer's definition)
// layer_2_name {\n
// master-tileSet-index, master-tileSet-index, ... master-tileSet-index\n
// }\n		(signifies end of a layer's definition)
// (repeat layer definitions for Num_Layers)
// }\n		(signifies end of ALL layers' definitions, moving on to entity map's entity definitions)
// [NOTE]: 0 as a master-tileSet-index indicates a placeholder, ie a tileMap index to skip for that layer
// [NOTE]: ALL master-tileSet-index read are reduced by 1 before loading into an eTileImpl::type
// # batch-load eEntity prefabs used on this map (defines prefabList indexes used below)\n
// # any number of leading comments with '#' between layer and entity definitions\n
// # only use [0|1] prefab batch file, to simplify entity map assignment\n
// Entity_Prefab_BatchFilename: entityPrefabBatchFilename.bprf\n
// Spawn_List_Title {\n
// prefabShortName: xPos yPos zPos	# eEntity::collisionModel::origin in orthographic 2D world-space, zPos of eEntity::renderImage::renderBlock's bottom in 3D world-space\n
// prefabShortName: xPos yPos zPos	# string float float float (everything past the last float is ignored)\n
// (repeat)
// }\n		(signifies end of the spawn list definition for this map)
//**************
bool eMap::LoadMap(const char * mapFilename) {
	std::ifstream	read(mapFilename);
	// unable to find/open file
	if (!read.good()) 
		return false;

	std::vector<eRenderImage *> sortTiles;
	char buffer[MAX_ESTRING_LENGTH];
	memset(buffer, 0, sizeof(buffer));

	int numColumns = 0;
	int numRows = 0;
	int cellWidth = 0;
	int cellHeight = 0;
	int numLayers = 0;
	int row = 0;
	int column = 0;

	while (read.peek() == '#')
		read.ignore(std::numeric_limits<std::streamsize>::max(), '\n');		// skip comments at the top of the file

	for (int i = 0; i < 6; ++i) {
		SkipFileKey(read);													// value label text
		switch (i) {
			case 0: read >> numColumns; break;
			case 1: read >> numRows;	break;
			case 2: read >> cellWidth;	break;
			case 3: read >> cellHeight; break;
			case 4: read >> numLayers;	break;
			case 5: read.getline(buffer, sizeof(buffer), '\n'); break;		// tileSet filename
		}

		read.ignore(std::numeric_limits<std::streamsize>::max(), '\n');		// skip the rest of the line
		if (!VerifyRead(read))
			return false;
	}	

	tileMap.SetGridSize(numRows, numColumns);					
	tileMap.SetCellSize(cellWidth, cellHeight);

	float mapWidth = (float)tileMap.Width();
	float mapHeight = (float)tileMap.Height();
	absBounds = eBounds(vec2_zero, eVec2(mapWidth, mapHeight));
	edgeColliders = { { {eBounds(vec2_zero, eVec2(0.0f, mapHeight)),				   vec2_oneZero},	// left
						{eBounds(eVec2(mapWidth, 0.0f), eVec2(mapWidth, mapHeight)),  -vec2_oneZero},	// right
						{eBounds(vec2_zero, eVec2(mapWidth, 0.0f)),					   vec2_zeroOne},	// top
						{eBounds(eVec2(0.0f, mapHeight), eVec2(mapWidth, mapHeight)), -vec2_zeroOne} }	// bottom
	};	

	// initialize each tileMap cell absBounds for image and collisionModel cell-occupancy tests
	for (column = 0; column < numColumns; ++column) {
		for (row = 0; row < numRows; ++row) {
			auto & cell = tileMap.Index(row, column);
			cell.SetGridPosition(row, column);
			eVec2 cellMins = eVec2((float)(row * cellWidth), (float)(column * cellHeight));
			cell.SetAbsBounds( eBounds(cellMins, cellMins + eVec2((float)cellWidth, (float)cellHeight)) );
			cell.TilesOwned().reserve(numLayers);	// BUGFIX: assures the tilesOwned vector data doesn't reallocate/move and invalidate tilesToDraw
		}
	}

	if (!eTileImpl::LoadTileset(buffer))
		return false;

	sortTiles.reserve(numRows * numColumns * numLayers);

	// READING LAYERS
	Uint32 layer = 0;
	read.ignore(std::numeric_limits<std::streamsize>::max(), '\n');			// ignore "Layers {\n"
	while (read.peek() != '}') {
		row = 0;
		column = 0;
		size_t tallestRenderBlock = 0;
		read.ignore(std::numeric_limits<std::streamsize>::max(), '\n');		// ignore "layer_# {\n"

		// read one layer
		while (read.peek() != '}') {
			int tileType = INVALID_ID;
			read >> tileType;
			if (!VerifyRead(read))
				return false;
			
			--tileType;			// DEBUG: .map format is easier to read with 0's instead of -1's so all values are incremented by 1 when writing it
			if (tileType > INVALID_ID) {
				auto & cell = tileMap.Index(row, column);
				auto & origin = cell.AbsBounds()[0];
				cell.AddTileOwned(eTile(&cell, origin, tileType, layer));
				auto & tileRenderImage = cell.TilesOwned().back().RenderImage();
				if (tileRenderImage.GetRenderBlock().Depth() > tallestRenderBlock)
					tallestRenderBlock = (size_t)tileRenderImage.GetRenderBlock().Depth();

				sortTiles.emplace_back(&tileRenderImage);
			}

			if (read.peek() == '\n') {
				read.ignore(1, '\n');
				row = 0;
				column++;
			} else if (read.peek() == ',') {
				read.ignore(1, ',');
				row++;
				if (row >= tileMap.Rows()) {
					row = 0;
					column++;
				}
			}

			if (!tileMap.IsValid(row, column))									// TODO: remove this, it's just a backup in case of an excessive .map file
				return false;
		}

		read.ignore(std::numeric_limits<std::streamsize>::max(), '\n');			// ignore layer closing brace '}\n'
		tileMap.AddLayerDepth(tallestRenderBlock);
		++layer;
	}
				
	read.ignore(std::numeric_limits<std::streamsize>::max(), '\n');				// ignore layers group closing brace '}\n'
						  
	// LOADING PREFABS
	while (read.peek() == '#')
		read.ignore(std::numeric_limits<std::streamsize>::max(), '\n');			// skip comments between the map layers and entity prefabs/spawning

	SkipFileKey(read);															// skip "Entity_Prefab_BatchFilename:"
	memset(buffer, 0, sizeof(buffer));
	read.getline(buffer, sizeof(buffer), '\n');
	if (!VerifyRead(read))
		return false;

	game.GetEntityPrefabManager().BatchLoad(buffer);							// DEBUG: any batch errors get logged, but doesn't stop the map from loading

							  
	// SPAWNING ENTITIES
	read.ignore(std::numeric_limits<std::streamsize>::max(), '\n');				// ignore "Spawn_List {\n"

	while (read.peek() != '}') {
		memset(buffer, 0, sizeof(buffer));
		read.getline(buffer, sizeof(buffer), ':');								// prefabShortName
		std::string prefabShortName(buffer);
		if (!VerifyRead(read))
			return false;

		eVec3 worldPosition;
		read >> worldPosition.x;
		read >> worldPosition.y;
		read >> worldPosition.z;
		read.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		if (!VerifyRead(read))
			return false;

		if (!game.GetEntityPrefabManager().SpawnInstance(prefabShortName, worldPosition)) {
			std::string message = "Invalid prefabShortName (";
			message += prefabShortName;
			message += "), or invalid prefab file contents.";
			EVIL_ERROR_LOG.LogError(message.c_str(), __FILE__, __LINE__);
		}
	}

	read.close();

	// initialize the static map images sort order
	eRenderer::TopologicalDrawDepthSort(sortTiles);	
	return true;
}

//***************
// eMap::UnloadMap
// clears the current tileMap and empties eGame::entities
//***************
void eMap::UnloadMap() {
	tileMap.ResetAllCells();
	game.ClearAllEntities();
}

//***************
// eMap::Draw
/*
FIXME: [minimize draw calls]
1.) at the end of eMap::Load, call an eRenderer fn to create and draw on a mapTexture of the entire map (allocate a texture the size of the map [big big])
2.) on eMap::Draw, set the visible cells like normal
3.) on eGridCell::Draw, only add those renderImages in gridcells containing an entity (eRenderImage::isSelectable)
4.) eRenderer::FlushCameraPool should behave as normal, AND prior to the first RenderCopy, copy the mapTexture to the scalable target (moved with camera)
RESULTS: does significantly reduce draw calls, but the layering visuals are wrong; also once eMovement pathing starts the fps still drops to 166-200 from 250-500
(removed this logic)
*/
//***************
void eMap::Draw() {
	auto & camera = game.GetCamera();
	if (camera.Moved() || game.GetGameTime() < 5000) {		// reduce visibleCells setup, except during startup
		visibleCells.clear();

/*
		// use the corner cells of the camera to designate the draw area
		// same speed in Release, several ms slower in debug
		auto & camBounds = camera.CollisionModel().AbsBounds();
		eVec2 corner = camBounds[0];
		eVec2 xAxis(camBounds[1].x, camBounds[0].y);
		eVec2 yAxis(camBounds[0].x, camBounds[1].y);
		std::array<eVec2, 3> obbPoints = { std::move(corner), std::move(xAxis), std::move(yAxis) };
		for (auto & point : obbPoints) { eMath::IsometricToCartesian(point.x, point.y); }
		eBox cameraArea(obbPoints.data());
		eCollision::GetAreaCells(cameraArea, visibleCells);
	}
*/
		// use the corner cells of the camera to designate the draw area
		const auto & camBounds = camera.CollisionModel().AbsBounds();
		std::array<eVec2, 4> corners;
		camBounds.ToPoints(corners.data());
		for (auto & point : corners) { eMath::IsometricToCartesian(point.x, point.y); }
		int startRow, startCol;
		int endRow, endCol;
		int finalRow, finalCol;
		tileMap.Index(corners[0], startRow, startCol);
		tileMap.Index(corners[1], endRow, endCol);
		tileMap.Index(corners[2], finalRow, finalCol);

		// DEBUG: magic numbers here based loosely on the way the camera area covers cells.
		// alternatively camBounds could be expanded by 128.0f prior to rotation, but that grabs too many cells
		// more horizontal cells
		startRow -= 2;	
		endRow += 1;	
	
		// more vertical cells
		finalCol += 3;	
		finalRow += 3;

		// staggered cell draw order
		// cells' tiles already have isometric coordinates
		int row = startRow;
		int column = startCol;
		bool oddLine = false;
		while (endRow <= finalRow || endCol <= finalCol) {	// shifting down the screen
			while(row <= endRow && column >= 0) {			// shifting across the screen
				if (tileMap.IsValid(row, column)) {
					auto & cell = tileMap.Index(row, column);
					cell.Draw();
					visibleCells.emplace_back(&cell);
				} 
				row++; column--;
			}
			oddLine = !oddLine;
			oddLine ? startCol++ : startRow++;
			oddLine ? endCol++ : endRow++;
			row = startRow;
			column = startCol;
		}
	} else {
		for (auto & cell : visibleCells)
			cell->Draw();
	}
}

//***************
// eMap::DebugDraw
//***************
void eMap::DebugDraw() { 
	for (auto & cell : visibleCells)
		cell->DebugDraw();
}
