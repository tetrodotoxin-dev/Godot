# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends SceneTree

const INVERT = "aa57de9d-d269-4e29-8892-5a047fbf0601"
const COMPOSITE = "aa57de9d-d269-4e29-8892-5a047fbf0603"
var failed := false

# Provider callbacks are real engine calls. They cannot change the renderer's
# selected operation halfway through producing its current source publication.
class ReentrantFactory extends RefCounted:
	var renderer: TtxRender
	func create_image(width: int, height: int, pixels: PackedByteArray) -> RefCounted:
		renderer.operation = INVERT
		return preload("res://addons/godot_ttx/gdscript/provider.gd").new().create_image(width, height, pixels)

class ProviderConfig extends TtxImageProvider:
	var factory: RefCounted
	func create_source() -> TtxImage:
		var source := TtxImage.new()
		source.set_provider_object(factory)
		return source

func require(condition: bool, message: String) -> void:
	if not condition:
		failed = true
		push_error(message)

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	# These are addon nodes in an ordinary scene. Both standard display classes
	# consume the same Texture2D without knowing about TTX Resources or providers.
	var scene := Node.new()
	root.add_child(scene)
	var source := TtxRender.new()
	var effect := TtxRender.new()
	scene.add_child(source)
	scene.add_child(effect)
	source.owner = scene
	effect.owner = scene
	source.name = "Input"
	effect.name = "Effect"
	var pixels := ImageTexture.create_from_image(Image.create_from_data(2, 1,
		false, Image.FORMAT_RGBA8, PackedByteArray([10, 20, 30, 255, 40, 50, 60, 127])))
	source.source_texture = pixels
	effect.input = source
	effect.request(INVERT)
	var rectangle := TextureRect.new()
	var sprite := Sprite2D.new()
	scene.add_child(rectangle)
	scene.add_child(sprite)
	var update := func():
		rectangle.texture = effect.get_texture()
		sprite.texture = rectangle.texture
	effect.output_changed.connect(update)
	update.call()
	require(rectangle.texture != null and sprite.texture == rectangle.texture, "Stock Godot nodes could not consume the renderer texture")
	require(rectangle.texture.get_image().get_data() == PackedByteArray([245, 235, 225, 255, 215, 205, 195, 127]), "Renderer changed the inversion promise")
	var retained := rectangle.texture
	source.source_texture = ImageTexture.create_from_image(Image.create_from_data(2, 1, false, Image.FORMAT_RGBA8, PackedByteArray([1, 2, 3, 255, 4, 5, 6, 255])))
	require(rectangle.texture.get_image().get_data()[0] == 254, "Input edits did not propagate through the node connection")
	require(retained.get_image().get_data()[0] == 245, "A retained Godot texture changed with its producer")

	# A visible policy limits later operations as well as the current renderer.
	# Removing the policy restores the scene edge, without replacing a backend.
	var policy := TtxRenderPolicy.new()
	policy.restrict_contracts = true
	policy.contracts = PackedStringArray([INVERT])
	source.policy = policy
	require(effect.admit(COMPOSITE).status == 3, "A derived renderer shed its input policy")
	policy.contracts = PackedStringArray()
	require(effect.get_texture() == null, "An empty restricted surface allowed an operation")
	source.policy = null
	require(effect.get_texture() != null, "Removing a scene policy did not restore the input edge")
	source.input = effect
	require(source.input == null and "cycle" in source.get_error(), "A cycle was admitted into renderer inputs")

	# Packing reconstructs the actual node links and private runtime images.
	# The resulting node can render before any lab controller exists.
	var packed := PackedScene.new()
	require(packed.pack(scene) == OK, "Renderer nodes could not be packed")
	var restored := packed.instantiate()
	root.add_child(restored)
	var restored_effect: TtxRender = restored.get_node("Effect")
	require(restored_effect.input == restored.get_node("Input"), "PackedScene did not restore the renderer reference")
	require(restored_effect.get_texture() != null, "Restored renderer had no output")
	restored.free()
	effect.output_changed.disconnect(update)
	source.free()
	require(effect.get_texture() == null, "A removed input remained a live scene relationship")
	require(retained.get_image().get_data()[0] == 245, "Node destruction revoked a retained Texture2D")
	var reentrant := TtxRender.new()
	var factory := ReentrantFactory.new()
	var provider := ProviderConfig.new()
	factory.renderer = reentrant
	provider.factory = factory
	scene.add_child(reentrant)
	reentrant.provider = provider
	reentrant.source_texture = pixels
	require(reentrant.get_texture() != null and reentrant.operation.is_empty(), "A provider changed renderer configuration during observation")
	scene.free()
	print("PASS renderer nodes: standard Texture2D consumers, scene propagation, policies, packing and destruction" if not failed else "FAIL renderer nodes")
	quit(1 if failed else 0)
