"""Check the renderer oracle references without running either renderer."""
import argparse
from copy import deepcopy
import json
from fractions import Fraction
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def require(condition, description):
    if not condition:
        raise ValueError(description)


BINDINGS_VERSION = "subathon.scene-bindings-conditions-timeline.v1"


def binding_ref(kind, resource):
    return {"kind": kind, "version": BINDINGS_VERSION,
            "resourceId": resource.encode("ascii").hex(), "definitionId": "7231"}


def geometry_decimal(value):
    value = Fraction(value)
    # Authored alignment overrides are exact quarter fractions.
    spellings = {Fraction(1, 4): "0.25", Fraction(3, 4): "0.75"}
    if value.denominator == 1:
        return str(value.numerator)
    return spellings[value]


def materialize(case, scene, source_by_id):
    source = source_by_id[scene["sceneCase"]]
    document = deepcopy(source_by_id[source.get("base", source["id"])]["instance"])
    bindings = deepcopy(source_by_id[scene["bindingsCase"]]["instance"]) if (
        "bindingsCase" in scene
    ) else {"version": BINDINGS_VERSION, "definitions": []}
    nodes = {node["nodeId"]: node for node in document["nodes"]}

    for change in source.get("changes", []):
        require(change["op"] == "replace" and change["path"] == "/requiredFeatures",
                "Unexpected stock fixture patch")
        document["requiredFeatures"] = deepcopy(change["value"])

    def attach(node, field, kind, resource, payload):
        ref = binding_ref(kind, resource)
        if field == "animations":
            node[field].append(ref)
        else:
            node[field] = ref
        document["externalReferences"].append(ref.copy())
        definition = {key: value for key, value in ref.items() if key != "version"}
        bindings["definitions"].append(definition | payload)

    if "condition" in case:
        condition = case["condition"]
        expression = {"kind": "all", "children": [
            {"kind": "comparison-phase", "operand": {"field": "timer.phase"},
             "operator": "eq", "literal": condition["phase"]},
            {"kind": "comparison-boolean", "operand": {"field": "pause.active"},
             "operator": "eq", "literal": condition["pauseActive"]},
        ]}
        attach(nodes[condition["nodeId"]], "visibility", "Condition", "fixture-grace-pause",
               {"expression": expression})

    if "canvas" in case:
        canvas = case["canvas"]
        width, height = canvas["size"]
        align_x, align_y = map(geometry_decimal, canvas["align"])
        document["canvas"] = {"width": width, "height": height, "fit": canvas["mode"],
                              "alignX": align_x, "alignY": align_y, "clip": canvas["clip"]}
        x, y, width, height = case["nodeFrame"]
        nodes[case["nodeId"]]["frame"].update(x=x, y=y, width=width, height=height)

    if "tracks" in case:
        node = nodes[case["nodeId"]]
        node["frame"]["x"], node["frame"]["y"] = case["baseMove"]
        for index, track in enumerate(case["tracks"], start=1):
            animation = {"kind": "move", "startMs": track["startMs"],
                         "durationMs": track["durationMs"], "easing": track["ease"],
                         "context": {"kind": "scene-start"},
                         "from": track["from"], "to": track["to"]}
            attach(node, "animations", "Animation", f"fixture-move-{index}", {"track": animation})

    if "nested" in case:
        nested = case["nested"]
        parent = nodes[nested["parent"]]
        child = nodes[nested["child"]]
        require(child["nodeId"] in parent["children"], "Nested child is not owned by group")
        parent["frame"].update(nested["parentFrame"])
        child["frame"].update(nested["childFrame"])

    if any(key in case for key in ("condition", "canvas", "tracks", "nested")) or "changes" in source:
        document["revisionId"] = case["id"].encode("ascii").hex()

    return document, bindings

def check(output=None):
    mapping = read(HERE / "scene-case-map.json")
    repo = HERE.parents[3]
    source = read(repo / mapping["sourceCases"])["cases"]
    source_by_id = {case["id"]: case for case in source}
    raw = read(HERE / "binding-oracles.json")
    observations = {item["case"]: item for group in ("progress", "phaseAndModifiers")
                    for item in raw[group]}
    cases = mapping["cases"]
    require(len({case["id"] for case in cases}) == len(cases), "Duplicate case ID")
    for case in cases:
        scene = mapping["scenes"][case["scene"]]
        source_case = source_by_id[scene["sceneCase"]]
        document, bindings = materialize(case, scene, source_by_id)
        nodes = {node["nodeId"]: node for node in document["nodes"]}
        for node_id in [row["nodeId"] for row in case.get("assertions", [])]:
            require(node_id in nodes, f"{case['id']}: unresolved node {node_id}")
        if "nodeId" in case:
            require(case["nodeId"] in nodes, f"{case['id']}: unresolved transform target")
        definitions = {(entry["kind"], entry["resourceId"], entry["definitionId"])
                       for entry in bindings["definitions"]}
        require(len(definitions) == len(bindings["definitions"]), "Duplicate binding identity")
        for node in nodes.values():
            refs = [node.get(field) for field in ("attachment", "visibility", "binding")]
            refs.extend(node["animations"])
            for ref in refs:
                if isinstance(ref, dict) and "definitionId" in ref:
                    key = (ref["kind"], ref["resourceId"], ref["definitionId"])
                    require(key in definitions, f"{case['id']}: unresolved binding {key}")
                    require(ref in document["externalReferences"],
                            f"{case['id']}: missing direct reference {key}")
        observation = case.get("observation")
        if isinstance(observation, str):
            require(observation in observations, f"{case['id']}: missing observation")
        if output:
            with (output / f"{case['id']}.json").open("x", encoding="utf-8") as target:
                json.dump({"document": document, "bindings": bindings, "case": case,
                           "observation": observations.get(observation) if isinstance(observation, str)
                           else observation}, target, indent=2)
                target.write("\n")
        for assertion in case.get("assertions", []):
            if assertion.get("fill", {}).get("state") == "available":
                fraction = observations[observation]["expect"]["fraction"]
                exact = Fraction(int(fraction["numerator"]), int(fraction["denominator"]))
                require(Fraction(assertion["fill"]["length"]) == 76 * exact,
                        f"{case['id']}: inconsistent fill length")
        if "syntheticCapability" in case:
            require(case["expect"] == source_case["expect"],
                    f"{case['id']}: unsupported scene verdict differs")


if __name__ == "__main__":
    try:
        parser = argparse.ArgumentParser(description=__doc__)
        parser.add_argument("--output", type=Path, help="Existing empty fixture export directory")
        args = parser.parse_args()
        if args.output is not None:
            require(args.output.is_dir() and not any(args.output.iterdir()),
                    "Fixture export requires an empty directory")
        check(args.output)
    except (KeyError, OSError, ValueError) as error:
        sys.exit(str(error))
