#!/usr/bin/env python3
import json
import os
import glob

def update_component_json(file_path):
    """Update a component JSON file to include the new metadata fields."""
    with open(file_path, 'r') as f:
        data = json.load(f)

    # Add new metadata fields if they don't exist
    if 'schemaVersion' not in data:
        data['schemaVersion'] = 1
    if 'techTier' not in data:
        data['techTier'] = 1
    if 'manufacturer' not in data:
        # Assign manufacturers based on component type
        if data.get('category') == 'Weapon':
            data['manufacturer'] = 'Armstrong Industries'
        elif data.get('category') == 'Shield':
            data['manufacturer'] = 'Nova Dynamics'
        elif data.get('category') == 'PowerPlant':
            data['manufacturer'] = 'Nova Dynamics'
        elif data.get('category') == 'MainThruster' or data.get('category') == 'ManeuverThruster':
            data['manufacturer'] = 'Stellar Forge'
        elif data.get('category') == 'Sensor':
            data['manufacturer'] = 'Quantum Sensors Inc.'
        elif data.get('category') == 'Cargo':
            data['manufacturer'] = 'Interstellar Logistics'
        elif data.get('category') == 'Support':
            data['manufacturer'] = 'Life Systems Corp'
        else:
            data['manufacturer'] = 'Generic Manufacturer'
    if 'factionRestrictions' not in data:
        data['factionRestrictions'] = []

    # Write back the updated JSON
    with open(file_path, 'w') as f:
        json.dump(data, f, indent=2)

def main():
    # Find all JSON files in assets/components/
    component_files = glob.glob('assets/components/*.json')

    for file_path in component_files:
        print(f"Updating {file_path}")
        update_component_json(file_path)

    print(f"Updated {len(component_files)} component files")

if __name__ == '__main__':
    main()