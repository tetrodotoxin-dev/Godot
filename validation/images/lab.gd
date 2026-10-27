extends SceneTree

func _initialize() -> void:
	call_deferred("verify")

func verify() -> void:
	var scene: Control = load("res://lab.tscn").instantiate()
	root.add_child(scene)
	await process_frame
	await process_frame
	var status: Label = scene.get_node("%Status")
	for renderer in scene.get_renderers(): print("RENDERER ", renderer.name, " ", renderer.get_error())
	if not status.text.contains("RENDERERS MATCH"):
		push_error("Initial lab comparison failed: " + status.text)
		quit(1)
		return
	var filters := get_nodes_in_group("lab_filters")
	var revisions := []
	for renderer in filters: revisions.append(renderer.get_revision())
	scene.move_overlay()
	for i in filters.size():
		if filters[i].get_revision() != revisions[i]:
			push_error("Overlay edit recomputed an unchanged filter")
			quit(1)
			return
	scene.next_sample()
	if not status.text.contains("RENDERERS MATCH"):
		push_error("Source update comparison failed: " + status.text)
		quit(1)
		return
	var mode: OptionButton = scene.get_node("%Mode")
	for i in mode.item_count:
		mode.select(i)
		scene.compare()
		if not status.text.contains("RENDERERS MATCH"):
			push_error("Lab mode comparison failed: " + status.text)
			quit(1)
			return
	print("TTX_LAB_SCENE_PASS")
	scene.queue_free()
	await process_frame
	quit()
