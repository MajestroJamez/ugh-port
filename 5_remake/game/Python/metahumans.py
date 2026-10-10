"""The game's MetaHumans (UghMetaHumans.h): the copters' pilot and the passengers of the logic's cargo looks 1 .. 3,
made with MetaHuman Creator in the editor from its presets (the plugin MetaHumanCharacter and its optional content,
"MetaHuman Creator Core Data"), as stone age people: wild hair, beards for the men, no outfit (the body whole: an
outfit cuts the body away under it; the game covers it with leaves, FUghMetaHuman). For each: a copy of the preset
(/Game/External/MetaHumans/Characters/<name>), its hair and beard, the face rig and the skin textures from Epic's
cloud (the editor's Epic login), the assembly (pipeline Optimized, quality Medium: hair cards, baked textures) to
/Game/External/MetaHumans/<name> (made anew) with the common assets in .../Common. Not in git
(Epic's content, licensed for Unreal Engine projects). Run by metahumans.ps1 in the editor (its texture graphs bake
the textures, which a commandlet cannot):

    UnrealEditor UghGame.uproject -ExecutePythonScript=Python/metahumans.py -RenderOffscreen

Writes Saved/Logs/UghMetaHumans.txt: a line per step, "OK" at the end when every character was built.
"""
import os
import traceback

import unreal

ROOT = "/Game/External/MetaHumans"
GROOMS = "/MetaHumanCharacter/Optional/Grooms/Bindings"
PRESETS = "/MetaHumanCharacter/Optional/Presets"
REPORT = os.path.join(unreal.Paths.project_saved_dir(), "Logs", "UghMetaHumans.txt")

# the slot of the outfit, left empty
OUTFITS = "Outfits"

# name: the preset, the grooms by slot (None: none), the hair's colour (parameters of its materials: melanin 0 blond
# .. 1 black, redness, white amount: grey): told apart from the game's camera (step 32c) - the man black, the woman
# light blond, the old man white
CHARACTERS = {
    "Pilot": ("Mateo", {"Hair": "Hair/WI_Hair_L_MessyClumps", "Beard": "Beards/WI_Beard_S_Stubble",
                        "Mustache": "Mustaches/WI_Mustache_S_Stubble"},
              {"hairMelanin": 0.8, "hairRedness": 0.15, "WhiteAmount": 0.0}),
    "Man": ("Bruce", {"Hair": "Hair/WI_Hair_M_Layered", "Beard": "Beards/WI_Beard_L_Messy",
                      "Mustache": "Mustaches/WI_Mustache_L_Messy"},
            {"hairMelanin": 0.95, "hairRedness": 0.1, "WhiteAmount": 0.0}),
    "Woman": ("Celeste", {"Hair": "Hair/WI_Hair_L_MessyClumps", "Beard": None, "Mustache": None},
              {"hairMelanin": 0.1, "hairRedness": 0.15, "WhiteAmount": 0.0}),
    "Grandpa": ("Walter", {"Hair": "Hair/WI_Hair_L_MessyClumps", "Beard": "Beards/WI_Beard_L_Full",
                           "Mustache": "Mustaches/WI_Mustache_L_Full"},
                {"hairMelanin": 0.2, "hairRedness": 0.0, "WhiteAmount": 1.0}),
}

lines = []


def report(text):
    lines.append(text)
    unreal.log(f"UghMetaHumans: {text}")
    with open(REPORT, "w", encoding="ascii", errors="replace") as file:
        file.write("\n".join(lines) + "\n")


def character_of(name, preset):
    """A fresh copy of the preset."""
    path = f"{ROOT}/Characters/{name}"
    library = unreal.EditorAssetLibrary
    if library.does_asset_exist(path):
        library.delete_asset(path)
    try:
        library.duplicate_asset(f"{PRESETS}/{preset}", path)
    except Exception as error:   # its control rigs log harmless errors, which Python raises
        report(f"  copy logged: {str(error).splitlines()[0][:200]}")
    character = unreal.load_asset(path)
    if character is None:
        raise RuntimeError(f"no preset {preset}")
    return character


def dress(subsystem, character, grooms):
    """The hair and beard of `grooms`, no outfit."""
    collection = subsystem.get_preview_collection(character)
    for slot, item in {**grooms, OUTFITS: None}.items():
        if item is None:
            collection.default_instance.set_single_slot_selection(slot_name=slot, item_key=unreal.MetaHumanPaletteItemKey())
            continue
        wardrobe = unreal.load_asset(f"{GROOMS}/{item}")
        if wardrobe is None:
            raise RuntimeError(f"no groom {item}")
        key = collection.try_add_item_from_wardrobe_item(slot_name=slot, wardrobe_item=wardrobe)
        collection.default_instance.set_single_slot_selection(slot_name=slot, item_key=key)
    subsystem.on_edit_preview_collection(character)


def build(name, preset, grooms):
    subsystem = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
    character = character_of(name, preset)
    if not subsystem.try_add_object_to_edit(character):
        raise RuntimeError("cannot edit the character")
    try:
        dress(subsystem, character, grooms)
        rig = unreal.MetaHumanCharacterAutoRiggingRequestParams()
        rig.blocking, rig.report_progress = True, False
        rig.rig_type = unreal.MetaHumanRigType.JOINTS_ONLY
        subsystem.request_auto_rigging(character, rig)
        textures = unreal.MetaHumanCharacterTextureRequestParams()
        textures.blocking, textures.report_progress = True, False
        subsystem.request_texture_sources(character, textures)
        if not subsystem.can_build_meta_human(character):
            raise RuntimeError("not ready for the assembly (no face rig or textures: the editor's Epic login?)")
        params = unreal.MetaHumanCharacterEditorBuildParameters()
        params.pipeline_type = unreal.MetaHumanDefaultPipelineType.OPTIMIZED
        params.pipeline_quality = unreal.MetaHumanQualityLevel.MEDIUM
        params.absolute_build_path = ROOT
        params.common_folder_path = f"{ROOT}/Common"
        params.enable_wardrobe_item_validation = False
        if unreal.EditorAssetLibrary.does_directory_exist(f"{ROOT}/{name}"):
            unreal.EditorAssetLibrary.delete_directory(f"{ROOT}/{name}")
        try:
            subsystem.build_meta_human(character, params)
        except Exception as error:   # the pipeline's control rigs log harmless errors, which Python raises
            report(f"  assembly logged: {str(error).splitlines()[0][:200]}")
    finally:
        subsystem.remove_object_to_edit(character)
    if not built(name):
        raise RuntimeError("the assembly made no body")
    if unreal.EditorAssetLibrary.does_directory_exist(f"{ROOT}/{name}/Clothing"):
        raise RuntimeError("the assembly made an outfit")


def retouch(name, hair):
    """The colour of the hair and the beard: the parameters of the assembled grooms' materials."""
    editing = unreal.MaterialEditingLibrary
    for data in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path(f"{ROOT}/{name}/Grooms", True):
        material = data.get_asset()
        if isinstance(material, unreal.MaterialInstanceConstant):
            for parameter, value in hair.items():
                editing.set_material_instance_scalar_parameter_value(material, parameter, value)
            editing.update_material_instance(material)
            unreal.EditorAssetLibrary.save_loaded_asset(material)


def built(name):
    return unreal.EditorAssetLibrary.does_asset_exist(f"{ROOT}/{name}/Body/SKM_{name}_BodyMesh")


def main():
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/MetaHumanCharacter", ROOT], True)
    again = "-UghAll" in unreal.SystemLibrary.get_command_line()
    done = 0
    for name, (preset, grooms, hair) in CHARACTERS.items():
        if built(name) and not again:
            report(f"{name}: there")
            retouch(name, hair)
            done += 1
            continue
        report(f"{name}: {preset}")
        try:
            build(name, preset, grooms)
            retouch(name, hair)
            done += 1
            report(f"{name}: built")
        except Exception:
            report(f"{name}: FAILED {traceback.format_exc()}")
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    report("OK" if done == len(CHARACTERS) else f"FAILED: {len(CHARACTERS) - done} not built")


try:
    main()
finally:
    unreal.SystemLibrary.quit_editor()
