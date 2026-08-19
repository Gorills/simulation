extends PanelContainer

@onready var initial_focus: Button = %PrimaryAction


func _ready() -> void:
    initial_focus.grab_focus.call_deferred()
