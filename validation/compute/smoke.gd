# Copyright (c) 2023-present Matt Kaes and contributors

extends Label

func _ready() -> void:
	if not ClassDB.class_exists("ComputeCpu"):
		fail("Compute factory was not discovered")
		return
	var cpu = ClassDB.instantiate("ComputeCpu")
	if cpu.has_method("allocate"):
		fail("Factory exposed a member Callable as static")
		return
	var first = cpu.create_engine()
	var second = cpu.create_engine()
	if first == null or second == null:
		fail("Factory did not return Engine values")
		return
	if first.has_method("create_engine"):
		fail("Engine exposed a static Callable as a member")
		return
	if ClassDB.can_instantiate("ComputeEngine") or ClassDB.can_instantiate("ComputeBuffer"):
		fail("A represented value can be constructed without a provider")
		return
	if first.allocate(0) != null:
		fail("An invalid allocation produced a Buffer")
		return
	var buffer = first.allocate(4096)
	var other = second.allocate(32)
	if buffer == null or other == null:
		fail("Engine allocation failed")
		return
	if first.release() != 0 or buffer.size() != 4096:
		fail("Buffer did not preserve its access after Engine release")
		return
	var expected := PackedByteArray([3, 1, 4, 1, 5, 9])
	if buffer.write(7, expected) != 0:
		fail("Buffer refused a canonical byte View")
		return
	var observed: PackedByteArray = buffer.read()
	if observed.slice(7, 7 + expected.size()) != expected:
		fail("Buffer readback did not preserve the written bytes")
		return
	var commands = second.create_commands()
	var command_input = second.allocate(8)
	var command_output = second.allocate(8)
	var command_source := PackedByteArray([10, 20, 30, 40, 50, 60, 70, 80])
	if commands == null or command_input == null or command_output == null:
		fail("Engine did not produce command resources")
		return
	if commands.wait() != 1:
		fail("Commands waited before submission")
		return
	if command_input.write(0, command_source) != 0:
		fail("Command input upload failed")
		return
	if commands.invert(command_input, command_output, 8) != 0:
		fail("Invert command was not recorded")
		return
	if commands.submit() != 0:
		fail("Recorded commands did not complete")
		return
	if commands.invert(command_input, command_output, 8) != 1:
		fail("Commands accepted recording after submission")
		return
	if commands.submit() != 1:
		fail("Commands accepted repeated submission")
		return
	if commands.wait() != 0 or commands.wait() != 0:
		fail("Submitted commands did not preserve their completion outcome")
		return
	var command_result: PackedByteArray = command_output.read()
	if command_result != PackedByteArray([245, 235, 225, 40, 205, 195, 185, 80]):
		fail("Recorded command produced incorrect bytes")
		return
	if commands.release() != 0 or command_input.release() != 0 or command_output.release() != 0:
		fail("Command resources did not release")
		return
	first = null
	if buffer.release() != 0 or other.size() != 32:
		fail("Independent Buffer acquisitions interfered")
		return
	buffer = null
	if other.release() != 0 or second.release() != 0:
		fail("Provider cleanup failed")
		return
	other = null
	second = null
	cpu = null
	var cuda_error := test_cuda()
	if not cuda_error.is_empty():
		fail(cuda_error)
		return
	text = "Typed Compute factory, Engine and Buffer passed"
	print("TTX_TYPED_COMPUTE_PASS")
	if DisplayServer.get_name() == "headless":
		get_tree().quit(0)

func test_cuda() -> String:
	if not ClassDB.class_exists("ComputeCuda"):
		return "CUDA Compute factory was not discovered"
	var factory = ClassDB.instantiate("ComputeCuda")
	var engine = factory.create_engine()
	if engine == null:
		return "CUDA Compute engine creation failed"
	var input = engine.allocate(8)
	var output = engine.allocate(8)
	var commands = engine.create_commands()
	if input == null or output == null or commands == null:
		return "CUDA Compute resources were not created"
	var source := PackedByteArray([10, 20, 30, 40, 50, 60, 70, 80])
	if input.write(0, source) != 0:
		return "CUDA Buffer upload failed"
	if commands.invert(input, output, 8) != 0:
		return "CUDA invert command was not recorded"
	if commands.submit() != 0 or commands.wait() != 0:
		return "CUDA invert command did not complete"
	var observed: PackedByteArray = output.read()
	if observed != PackedByteArray([245, 235, 225, 40, 205, 195, 185, 80]):
		return "CUDA invert command produced incorrect bytes"
	if commands.release() != 0 or input.release() != 0 or output.release() != 0:
		return "CUDA command resources did not release"
	if engine.release() != 0:
		return "CUDA Compute engine did not release"
	return ""

func fail(message: String) -> void:
	text = message
	push_error(message)
	get_tree().quit(1)
