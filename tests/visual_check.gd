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
    var before: PackedByteArray = lab.cpu_view.texture.get_image().get_data()
    var cpu_blur: TtxImage = lab.cpu.filtered
    var cuda_blur: TtxImage = lab.cuda.filtered
    var script_blur: TtxImage = lab.scripted.filtered
    if not await _capture(4):
        return

    # Emit the actual button signal. Merely rerunning comparison would create
    # another graph and miss the promise that an existing result follows edits.
    for step in 5:
        lab.get_node("%MoveOverlay").pressed.emit()
        if lab.cpu.filtered != cpu_blur or lab.cuda.filtered != cuda_blur or lab.scripted.filtered != script_blur:
            _fail("Moving the overlay replaced a blur Resource")
            return
        if not _agree():
            return
        if before == lab.cpu_view.texture.get_image().get_data():
            _fail("Moving the overlay did not change the displayed pixels")
            return
        if not await _capture(2):
            return

    # A source edit should affect the wider branch. The same script and
    # rendering controls continue to consume both provider implementations.
    lab.get_node("%ChangeSource").pressed.emit()
    if lab.cpu.filtered == cpu_blur or lab.cuda.filtered == cuda_blur or lab.scripted.filtered == script_blur:
        _fail("Changing source did not replace the affected blur results")
        return
    if not _agree():
        return
    if not await _capture(4):
        return
    lab.get_node("%Mode").select(1)
    lab.get_node("%Compare").pressed.emit()
    if not _agree() or not await _capture(4):
        return
    print("PASS visual: GDScript/CPU/CUDA agreement, retained blur on overlay edit, source invalidation")
    quit()

func _agree() -> bool:
    if lab.cpu.composite == null or lab.cuda.composite == null or lab.scripted.composite == null:
        _fail("All providers must run: %s / %s / %s" % [lab.cpu.error, lab.cuda.error, lab.scripted.error])
        return false

    var reference: PackedByteArray = lab.cpu_view.texture.get_image().get_data()
    for view in [lab.cuda_view, lab.script_view]:
        var pixels: PackedByteArray = view.texture.get_image().get_data()
        if reference.is_empty() or reference.size() != pixels.size():
            _fail("Displayed image extents disagree")
            return false
        var maximum := 0
        for index in pixels.size():
            maximum = maxi(maximum, absi(reference[index] - pixels[index]))
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
