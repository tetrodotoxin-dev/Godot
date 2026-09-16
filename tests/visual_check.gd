# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends SceneTree

# The same scene is rendered into a fixed viewport so CI captures and the
# README animation do not depend on the developer's desktop window tiling.
# This exercises real controls and textures; native tests separately count
# device transfers and prove that underlying blur allocations remain unchanged.
var viewport: SubViewport
var lab: Control
var capture_directory := ""
var frame_index := 0

func _initialize() -> void:
    for argument in OS.get_cmdline_user_args():
        if argument.begins_with("--capture="):
            capture_directory = argument.trim_prefix("--capture=")
    _run.call_deferred()

func _run() -> void:
    viewport = SubViewport.new()
    viewport.size = Vector2i(1440, 760)
    viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
    root.add_child(viewport)

    var display := TextureRect.new()
    display.texture = viewport.get_texture()
    display.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
    display.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
    display.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
    root.add_child(display)

    lab = load("res://lab.tscn").instantiate()
    viewport.add_child(lab)
    if lab.get_script() == null:
        _fail("The lab script did not load")
        return
    while not lab.initialized:
        await process_frame

    if not _agree():
        return
    var renderers: Array[Node] = lab.get_renderers()
    var before: PackedByteArray = renderers[0].get_texture().get_image().get_data()
    var revisions: Array = []
    for renderer: TtxRender in renderers:
        revisions.append(renderer.input.get_revision())
    if not await _capture(4):
        return

    # Editing the shared overlay invalidates the connected output nodes. The
    # filter nodes keep their observations, and the panels update from signals.
    for step in 5:
        lab.get_node("%MoveOverlay").pressed.emit()
        if not _agree():
            return
        for index in renderers.size():
            if renderers[index].input.get_revision() != revisions[index]:
                _fail("An overlay edit recomputed an upstream filter")
                return
        if before == renderers[0].get_texture().get_image().get_data():
            _fail("The connected overlay did not change output pixels")
            return
        if not await _capture(2):
            return

    lab.get_node("%ChangeSource").pressed.emit()
    if not _agree():
        return
    for index in renderers.size():
        if renderers[index].input.get_revision() <= revisions[index]:
            _fail("A source edit failed to recompute a connected filter")
            return
    if not await _capture(4):
        return

    # Duplicate a configured renderer subtree and reuse the same panel scene.
    # The existing controller discovers its groups without a fourth code path.
    var extra: Node = renderers[0].get_parent().duplicate()
    extra.name = "AdditionalRenderer"
    lab.get_node("Renderers").add_child(extra)
    var panel: Control = load("res://preview.tscn").instantiate()
    panel.renderer = extra.get_node("Output")
    lab.get_node("Margin/Page/Previews").add_child(panel)
    lab.get_node("%Compare").pressed.emit()
    if not _agree(4):
        return
    panel.free()
    extra.free()

    # A new project effect has its own UUID. Availability is observed from
    # nodes rather than inferred from implementation labels in the controller.
    var modes: OptionButton = lab.get_node("%Mode")
    var found := false
    for index in modes.item_count:
        if modes.get_item_metadata(index).contract == "b3ca4a5b-3a72-4f17-861c-24b37bf70001":
            modes.select(index)
            found = true
    if not found:
        _fail("The project effect was not discovered")
        return
    lab.get_node("%Compare").pressed.emit()
    var available := 0
    for renderer: TtxRender in renderers:
        if renderer.get_texture() != null:
            available += 1
    if available != 1:
        _fail("Renderer nodes did not preserve their separate capabilities")
        return
    if not await _capture(4):
        return
    print("PASS visual: scene renderer composition, native/script agreement, retained upstream filters and discovered project effect")
    quit()

func _agree(expected: int = 3) -> bool:
    var reference := PackedByteArray()
    var renderers: Array[Node] = lab.get_renderers()
    if renderers.size() != expected:
        _fail("The scene did not publish its configured output renderers")
        return false
    for renderer: TtxRender in renderers:
        var texture := renderer.get_texture()
        if texture == null:
            _fail("A renderer could not produce a texture: " + renderer.get_error())
            return false
        var pixels := texture.get_image().get_data()
        if reference.is_empty():
            reference = pixels
        if reference.is_empty() or reference.size() != pixels.size():
            _fail("Displayed image extents disagree")
            return false
        var maximum := 0
        for index in pixels.size():
            maximum = maxi(maximum, absi(pixels[index] - reference[index]))
        if maximum > 1:
            _fail("Displayed values differ by more than one byte")
            return false
    return true

func _capture(repeats: int) -> bool:
    await RenderingServer.frame_post_draw
    if capture_directory.is_empty():
        return true
    var image := viewport.get_texture().get_image()
    for count in repeats:
        var path := capture_directory.path_join("%03d.png" % frame_index)
        if image.save_png(path) != OK:
            _fail("Could not save the actual rendered frame")
            return false
        frame_index += 1
    return true

func _fail(message: String) -> void:
    push_error(message)
    quit(1)
