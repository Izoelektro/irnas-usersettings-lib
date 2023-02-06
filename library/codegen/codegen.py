import argparse
import json
from pathlib import Path
from jsonschema import validate

def full_type_name(type):
    outputType = ""

    x = type.find(" ")             # remove " = 0"
    if x >= 0:
        type = type[0:x]

    x = type.rfind("_")            # remove "USER_SETTINGS_TYPE_"
    type = type[x+1:]

    type = type.lower()
    
    firstLetter = type[0]          
    if firstLetter == "u":
        outputType = firstLetter + "int" + type[1:] + "_t"
    elif firstLetter == "i":
        outputType = firstLetter + "nt" + type[1:] + "_t"
    elif type == "str":
        outputType = "string"
    elif type == "bool":
        outputType = "boolean"
    else:
        outputType = type
        #NOTE_to_dev: if other types need processing, add needed clauses

    return outputType


def main():
    parser = argparse.ArgumentParser(
        prog = 'Code generator',
        description= 'Create .h,.c files with user functions',
        epilog = 'support me on patreon'
    )
    parser.add_argument('Filename')
    args = parser.parse_args()

    if not args.Filename.endswith(".json"):
        print("Error: Input file must be of type: JSON(.json)")
        return
        # NOTE: we don't want the program to exit. To-do fix in future

    if not Path(args.Filename).is_file():
        print("Error: File not found.")
        return
        #NOTE same as note above

    f = open(args.Filename)
    data = json.load(f)
    #print(data)

#    f = open("../include/user_settings_types.h")
    #for line in open("../include/user_settings_types.h", 'r'):

 #   print(f.load())
  #  f.close

    allTypes = []
    with open("../include/user_settings_types.h", 'r') as fileHandle:
        fileContent = fileHandle.read()
        fileContent = fileContent[fileContent.find("enum user_setting_type {"):]
        fileContent = fileContent[fileContent.find("USER_SETTINGS"):fileContent.find("};")]
        fileContent = fileContent.replace("\t","")
        fileContent = fileContent.replace(",\n", ",") 
        typesArray = fileContent.split(",\n")   # split by type

        for types in typesArray:                # sub-split by number of bytes
            types = types.split(",")
            types = [x for x in types if x]     # catch empty strings
            
            for type in types:                  
                type = full_type_name(type)     # clean up unnecessary characters
                allTypes.append(type)


    schema = {
        "type": "object",
        "properties": {
            "version": {"type":"number"},
            "settings": {
                "type":"array",
                "items":{
                    "type":"object",
                    "patternProperties":{
                        ".*":{
                            "type":"object",
                            "properties": {
                                "id":{
                                    "type":"string"
                                },
                                "type":{
                                    "type":"string",
                                    "enum":allTypes
                                },
                                "default":{
                                    "type":["number", "string", "boolean", "array"]
                                }
                            },
                            "required":["id","type","default"],
                            "if":{
                                "properties": {
                                    "type":{
                                        "oneOf":[
                                            {"type":"string", "const":"string"},
                                            {"type":"string", "const":"bytes"}
                                        ]
                                    }
                                }
                            },
                            "then": {
                                "properties":{
                                    "size":{
                                        "type":"number"
                                    }
                                }
                            },
                            "else": {
                                "not":{
                                    "required": ["size"]
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    schema1 = {
        "type": "object",
        "properties": {
            "version": {"type":"number"},
            "settings": {
                "type":"object",
                "patternProperties":{
                    ".*":{
                        "type":"object",
                        "properties": {
                            "id":{
                                "type":"string"
                            },
                            "type":{
                                "type":"string",
                                "enum":allTypes
                            },
                            "default":{
                                "type":["number", "string", "boolean", "array"]
                            }
                        },
                        "required":["id","type","default"],
                        "if":{
                            "properties": {
                                "type":{
                                    "oneOf":[
                                        {"type":"string", "const":"string"},
                                        {"type":"string", "const":"bytes"}
                                    ]
                                }
                            }
                        },
                        "then": {
                            "properties":{
                                "size":{
                                    "type":"number"
                                }
                            }
                        },
                        "else": {
                            "not":{
                                "required": ["size"]
                            }
                        }
                    }
                }
            }
        }
    }
    validate(instance=data,schema=schema)  
                                            #object with settings = schema1 -> test1
                                            #array of settings = schema -> test
    #TODO: 
    # write json schema
    # read json file
    # validate data 
    # DATA VALIDATED


    # & put in arrays
    # start .c .h generation

if __name__ == "__main__":
    main()

