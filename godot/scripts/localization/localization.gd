extends Node

signal locale_changed(locale: String)

const DEFAULT_LOCALE := "ru"
const SUPPORTED_LOCALES := ["ru", "en"]


func _enter_tree() -> void:
    var requested_locale := _user_arg_value("--locale")
    if requested_locale.is_empty():
        requested_locale = DEFAULT_LOCALE

    if set_locale(requested_locale):
        return

    push_warning(
        "unsupported locale '%s'; falling back to project default '%s'"
        % [requested_locale, DEFAULT_LOCALE]
    )
    var restored_default := set_locale(DEFAULT_LOCALE)
    assert(restored_default)


func set_locale(locale: String) -> bool:
    var language := _language_code(locale)
    if not SUPPORTED_LOCALES.has(language):
        return false

    if current_locale() == language:
        return true

    TranslationServer.set_locale(language)
    locale_changed.emit(language)
    return true


func current_locale() -> String:
    return _language_code(TranslationServer.get_locale())


func supported_locales() -> Array:
    return SUPPORTED_LOCALES.duplicate()


func _language_code(locale: String) -> String:
    var standardized := TranslationServer.standardize_locale(locale)
    return standardized.get_slice("_", 0).get_slice("@", 0)


func _user_arg_value(name: String) -> String:
    var args := OS.get_cmdline_user_args()
    for index in range(args.size() - 1):
        if args[index] == name:
            return args[index + 1]
    return ""
