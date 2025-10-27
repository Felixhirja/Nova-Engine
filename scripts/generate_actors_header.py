#!/usr/bin/env python3
"""
Automatic actor header generator for Nova Engine.
Scans the actors directory and generates Actors.h with all actor includes.
"""

import os
import glob
from pathlib import Path

def generate_actors_header():
    """Generate the Actors.h file with all actor headers included."""

    # Get the directory where this script is located
    script_dir = Path(__file__).parent
    actors_dir = script_dir.parent / "actors"  # Go up one level to project root, then to actors
    engine_dir = script_dir.parent / "engine"  # Go up one level to project root, then to engine

    # Find all .h files in the actors directory
    actor_headers = []
    if actors_dir.exists():
        actor_headers = sorted(glob.glob(str(actors_dir / "*.h")))

    # Generate the Actors.h content
    content = ['#pragma once', '']
    content.append('// Auto-generated file - do not edit manually')
    content.append('// Run scripts/generate_actors_header.py to regenerate')
    content.append('')
    content.append('// Include all actor headers to ensure automatic registration')

    for header_path in actor_headers:
        # Get relative path from engine directory
        header_rel_path = os.path.relpath(header_path, engine_dir).replace('\\', '/')
        content.append(f'#include "{header_rel_path}"')

    content.append('')

    # Write the file
    actors_h_path = engine_dir / "Actors.h"
    with open(actors_h_path, 'w') as f:
        f.write('\n'.join(content))

    print(f"Generated {actors_h_path} with {len(actor_headers)} actor headers")

if __name__ == "__main__":
    generate_actors_header()