// main gui object
// GuiMain
// ###### -

// include header
#include "vibe/common/error.hxx"
#include "GuiMain.hxx"


namespace vibe {

// constructor
/*
GuiMain::GuiMain() {
    // wrap

}
*/

//GuiMain::GuiMain(guild::Args arguments) {
GuiMain::GuiMain() {
	printf("guimain constructor\n");

	// initialize mesher
	_mesher = Mesher::create();

	printf("end of constructor GuiMain\n");
}

// destructor
GuiMain::~GuiMain() {}

// factory
ShGuiMainPr GuiMain::create() {
	return std::make_shared<GuiMain>();
}

void GuiMain::load_objects() {

	// setup camera
	// @hey somewhere outside load objects
	_cameraObject.setParent(&_scene).translate(Vector3::zAxis(10.0f));
	(*(_camera = new SceneGraph::Camera3D{_cameraObject}))
		.setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
		//	.setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.01f, 1000.0f))
		.setProjectionMatrix(Matrix4::perspectiveProjection(45.0_degf, 1.0f, 0.1f, 1000.0f))
		.setViewport(GL::defaultFramebuffer.viewport().size());

	// ?
	_manipulator.setParent(&_scene);

	/* Setup renderer and shader defaults */
	// already set in imgui?
	GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
	GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);


	/* Load a scene importer plugin */
	printf("scene importer\n");
	PluginManager::Manager<Trade::AbstractImporter> manager;
	Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate("AnySceneImporter");

	// shaders
	_coloredShader.setAmbientColor(0x111111_rgbf).setSpecularColor(0xffffff_rgbf).setShininess(80.0f);
	_texturedShader.setAmbientColor(0x111111_rgbf).setSpecularColor(0x111111_rgbf).setShininess(80.0f);

	// import our file
	printf("file import\n");
	std::string fpath = "assets/models/spaceship3.obj";

	// asset files to load

	if(!importer || !importer->openFile(fpath)) std::exit(1);

	/* Load all textures. Textures that fail to load will be NullOpt. */
	printf("textures\n");
	_textures = Containers::Array<Containers::Optional<GL::Texture2D>>{importer->textureCount()};
	for(UnsignedInt i = 0; i != importer->textureCount(); ++i) {
		Containers::Optional<Trade::TextureData> textureData = importer->texture(i);
		if(!textureData || textureData->type() != Trade::TextureType::Texture2D) {
			Warning{} << "Cannot load texture" << i << importer->textureName(i);
			continue;
		}

		Containers::Optional<Trade::ImageData2D> imageData = importer->image2D(textureData->image());
		if(!imageData || !imageData->isCompressed()) {
			Warning{} << "Cannot load image" << textureData->image() << importer->image2DName(textureData->image());
			continue;
		}

		(*(_textures[i] = GL::Texture2D{}))
			.setMagnificationFilter(textureData->magnificationFilter())
			.setMinificationFilter(textureData->minificationFilter(), textureData->mipmapFilter())
			.setWrapping(textureData->wrapping().xy())
			.setStorage(
				Math::log2(imageData->size().max()) + 1, GL::textureFormat(imageData->format()), imageData->size())
			.setSubImage(0, {}, *imageData)
			.generateMipmap();
	}

	/* Load all materials. Materials that fail to load will be NullOpt. Only a
       temporary array as the material attributes will be stored directly in
       drawables later. */
	printf("materials\n");
	Containers::Array<Containers::Optional<Trade::PhongMaterialData>> materials{importer->materialCount()};
	for(UnsignedInt i = 0; i != importer->materialCount(); ++i) {
		Containers::Optional<Trade::MaterialData> materialData;
		if(!(materialData = importer->material(i))) {
			Warning{} << "Cannot load material" << i << importer->materialName(i);
			continue;
		}

		materials[i] = std::move(*materialData).as<Trade::PhongMaterialData>();
	}

	/* Load all meshes. Meshes that fail to load will be NullOpt. Generate
       normals if not present. */
	printf("meshes\n");
	_meshes = Containers::Array<Containers::Optional<GL::Mesh>>{importer->meshCount()};

	for(UnsignedInt i = 0; i != importer->meshCount(); ++i) {
		Containers::Optional<Trade::MeshData> meshData;
		if(!(meshData = importer->mesh(i))) {
			Warning{} << "Cannot load mesh" << i << importer->meshName(i);
			continue;
		}

		MeshTools::CompileFlags flags;
		if(!meshData->hasAttribute(Trade::MeshAttribute::Normal)) flags |= MeshTools::CompileFlag::GenerateFlatNormals;
		_meshes[i] = MeshTools::compile(*meshData, flags);
		_meshes2.push_back(MeshTools::compile(*meshData, flags));
	}

	/* The format has no scene support, display just the first loaded mesh with
       a default material (if it's there) and be done with it. */
	printf("default scene\n");
	if(importer->defaultScene() == -1) {
		if(!_meshes.isEmpty() && _meshes[0])
			new ColoredDrawable{_manipulator, _coloredShader, *_meshes[0], 0xffffff_rgbf, _drawables};
		return;
	}

	/* Load the scene */
	printf("load scene\n");
	Containers::Optional<Trade::SceneData> scene;
	if(!(scene = importer->scene(importer->defaultScene())) || !scene->is3D() ||
		!scene->hasField(Trade::SceneField::Parent) || !scene->hasField(Trade::SceneField::Mesh)) {
		Fatal{} << "Cannot load scene" << importer->defaultScene() << importer->sceneName(importer->defaultScene());
	}

	/* Allocate objects that are part of the hierarchy */
	printf("objects\n");
	Containers::Array<Object3D*> objects{std::size_t(scene->mappingBound())};
	Containers::Array<Containers::Pair<UnsignedInt, Int>> parents = scene->parentsAsArray();
	for(const Containers::Pair<UnsignedInt, Int>& parent : parents)
		objects[parent.first()] = new Object3D{};

	/* Assign parent references */
	for(const Containers::Pair<UnsignedInt, Int>& parent : parents)
		objects[parent.first()]->setParent(parent.second() == -1 ? &_manipulator : objects[parent.second()]);

	/* Set transformations. Objects that are not part of the hierarchy are
       ignored, objects that have no transformation entry retain an identity
       transformation. */
	printf("transforms\n");
	for(const Containers::Pair<UnsignedInt, Matrix4>& transformation : scene->transformations3DAsArray()) {
		if(Object3D* object = objects[transformation.first()]) object->setTransformation(transformation.second());
	}

	/* Add drawables for objects that have a mesh, again ignoring objects that
       are not part of the hierarchy. There can be multiple mesh assignments
       for one object, simply add one drawable for each. */
	printf("drawables\n");
	for(const Containers::Pair<UnsignedInt, Containers::Pair<UnsignedInt, Int>>& meshMaterial :
		scene->meshesMaterialsAsArray()) {
		Object3D* object = objects[meshMaterial.first()];
		Containers::Optional<GL::Mesh>& mesh = _meshes[meshMaterial.second().first()];
		if(!object || !mesh) continue;

		Int materialId = meshMaterial.second().second();

		/* Material not available / not loaded, use a default material */
		if(materialId == -1 || !materials[materialId]) {
			new ColoredDrawable{*object, _coloredShader, *mesh, 0xffffff_rgbf, _drawables};

			/* Textured material, if the texture loaded correctly */
		} else if(materials[materialId]->hasAttribute(Trade::MaterialAttribute::DiffuseTexture) &&
			_textures[materials[materialId]->diffuseTexture()]) {
			new TexturedDrawable{
				*object, _texturedShader, *mesh, *_textures[materials[materialId]->diffuseTexture()], _drawables};

			/* Color-only material */
		} else {
			new ColoredDrawable{*object, _coloredShader, *mesh, materials[materialId]->diffuseColor(), _drawables};
		}
	}
}

void GuiMain::show_objects() {

	// toggles
	static bool flag_toggle_hitboxes = true;
	static bool flag_toggle_meshes = true;
	static bool flag_toggle_wireframe;
	static bool flag_toggle_orientations = true;
	static Color4 color4;

	static struct {
		float hue = 180;
		float saturation = 0.5;
		float light = 1.0;
	} color_ambient;

	static struct {
		float hue = 30;
		float saturation = 0.5;
		float light = 1.0;
	} color_diffuse;


	// ImGui options?
	if(ImGui::Begin("Objects")) {
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
			1000.0 / Double(ImGui::GetIO().Framerate),
			Double(ImGui::GetIO().Framerate));

		// set change in time
		_dtt = ImGui::GetIO().DeltaTime;
		ImGui::Text("Delta Time [ms]: %0.3f\n", _dtt * 1000);

		ImGui::Text("GUI WINDOW\n");

		// walk objects
		for(size_t i = 0; i < _obj_list.size(); i++) {
			arma::Col<fltp> Rn = _obj_list[i]->_Rn;
			ImGui::Text("Obj(%lu) (x,y,z): (%0.2f,%0.2f,%0.2f)\n", i, Rn(0), Rn(1), Rn(2));
		}

		// draw the transformation matrices?
		// could help debug yes

		ImGui::Text("Projection matrix\n");
		print_matrix(_mesher->_projection);

		if(ImGui::Button("toggle meshes")) {
			flag_toggle_meshes ^= true;
		}
		if(ImGui::Button("toggle hitboxes")) {
			flag_toggle_hitboxes ^= true;
		}
		if(flag_toggle_hitboxes) ImGui::Text("Show hitboxes\n");

		// wireframe
		if(ImGui::Button("toggle wireframe")) {
			flag_toggle_wireframe ^= true;
		}
		if(flag_toggle_wireframe) ImGui::Text("Show wireframe\n");

		if(ImGui::Button("toggle orientations")) {
			flag_toggle_orientations ^= true;
		}

		// add a coloring option

		//static float hue, saturation, light;

		ImGui::Text("Lighting options\n");
		ImGui::Text("Ambient Color\n");
		ImGui::SliderFloat("hue##1", &color_ambient.hue, 0, 360);
		ImGui::SliderFloat("saturation##1", &color_ambient.saturation, 0, 1);
		ImGui::SliderFloat("light##1", &color_ambient.light, 0, 1);
		ImGui::Text("Diffuse Color\n");
		ImGui::SliderFloat("hue##2", &color_diffuse.hue, 0, 360);
		ImGui::SliderFloat("saturation##2", &color_diffuse.saturation, 0, 1);
		ImGui::SliderFloat("light##2", &color_diffuse.light, 0, 1);
		ImGui::Text("Position\n");
		ImGui::Text("Number of lights\n");

		// create our magnum color
		//color4 = Color4::fromHsv({hue * 1.0_degf, saturation, light});
	}
	ImGui::End();

	// update object positions
	for(size_t i = 0; i < _obj_list.size(); i++) {
		// this obj
		ShObjDataPr myobj = _obj_list[i];

		// update each
		myobj->update_kinematics(_dtt);
	}

	// check object interactions
	for(size_t i = 0; i < _obj_list.size(); i++) {

		// src obj
		ShObjDataPr objsrc = _obj_list[i];

		for(size_t j = 0; j < _obj_list.size(); j++) {

			// tar obj
			ShObjDataPr objtar = _obj_list[j];

			// check if overlap

			// check if inside

			// recalculate new position
			// based on incident direction vector
		}
	}

	//	VIBE_DEBUG("draw objects\n");

	// log debug
	_lg->msg("draw objects\n");

	// draw objects?
	// walk over objects
	for(size_t i = 0; i < _obj_list.size(); i++) {

		// this obj
		ShObjDataPr myobj = _obj_list[i];

		// combined matrix
		//Matrix4 transformation = _mesher->_transformation;
		Matrix4 transformation = _mesher->_transformation * myobj->get_translation_matrix();

		// add rotation?
		transformation = transformation * myobj->get_rotation_matrix();

		//Matrix4 projection = _mesher->_projection + myobj->get_translation_matrix();
		Matrix4 projection = _mesher->_projection;

		using namespace Magnum::Math::Literals;

		// lights!
		_shader.setLightPositions({{0.0f, 0.0f, 5.00f, 1.0f}});
		Color4 color_use;
		if(i == 0)
			color_use = Color4::fromHsv({color_ambient.hue * 1.0_degf, color_ambient.saturation, color_ambient.light});
		else
			color_use = Color4::fromHsv({1.0_degf, 0.0, 1.0});

		_shader.setAmbientColor(color_use);

		_shader.setDiffuseColor(
			Color4::fromHsv({color_diffuse.hue * 1.0_degf, color_diffuse.saturation, color_diffuse.light}));

		_shader.setTransformationMatrix(transformation);
		_shader.setNormalMatrix(transformation.normalMatrix());
		_shader.setProjectionMatrix(projection);
		//_shader.setTransformationMatrix(_mesher->_transformation + myobj->get_translation_matrix());
		//_shader.setNormalMatrix(_mesher->_transformation.normalMatrix());

		// draw
		//_shader.draw(myobj->_meshes.at(0));
		if(flag_toggle_meshes) {
			_shader.draw(myobj->_mesh);
		}

		// custom color mesh
		//_shader.draw(myobj->_mesh2);

		// if has hitbox
		if(flag_toggle_hitboxes) {
			//_shader.draw(myobj->get_hitbox_mesh());

			// draw wireframe
			_shader.draw(myobj->_hitbox->_mesh_wireframe);

			// if using sphere wireframe?
			// @hey, add separate option
			//	_shader.draw(myobj->_hitbox->_mesh_sphere);
			_shader.draw(myobj->_hitbox->_mesh_sphere_wireframe);
		}

		if(flag_toggle_wireframe) {
			//_shader.draw(myobj->get_wireframe_mesh());
		}

		// orientations
		if(flag_toggle_orientations) {
			// draw axes
			_shader.draw(myobj->_mesh_orientations);
		}
	} // draw objects

	//VIBE_DEBUG("done loop\n");
}


// setters
void GuiMain::set_noisemaker(daw::ShNoiseMakerPr nm) {
	_nm = nm;
}

void GuiMain::set_object(ShObjDataPr obj) {
	// clear list of objects
	_obj_list.clear();
	_obj_list.push_back(obj);
	_num_objects = 1;
}

void GuiMain::add_object(ShObjDataPr obj) {
	_obj_list.push_back(obj);
	_num_objects++;
}

Vector3 GuiMain::positionOnSphere(const Vector2i& position) const {
	const Vector2 positionNormalized = Vector2{position} / Vector2{_camera->viewport()} - Vector2{0.5f};
	const Float length = positionNormalized.length();
	const Vector3 result(
		length > 1.0f ? Vector3(positionNormalized, 0.0f) : Vector3(positionNormalized, 1.0f - length));
	return (result * Vector3::yScale(-1.0f)).normalized();
}

void GuiMain::print_matrix(Magnum::Matrix4 matrix) {
	//ImGui::Text("(%0.2f,%0.2f,%0.2f,%0.2f)\n",matrix.ij(0,0));

	// walk over rows?
	for(int i = 0; i < 4; i++) {
		Vector4 vtemp = matrix.row(i);
		ImGui::Text("(%0.2f,%0.2f,%0.2f,%0.2f)\n", vtemp.w(), vtemp.x(), vtemp.y(), vtemp.z());
	}
}

// getters

// static
} // namespace vibe
