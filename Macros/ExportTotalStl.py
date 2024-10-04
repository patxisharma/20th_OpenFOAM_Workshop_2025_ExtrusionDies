# Author: João Vidal
# Institution: Universidade do Minho
# Email: jpovidal@gmail.com
# Date: 2024-09-25
# Description: This script exports all bodies in the active document to .stl files, concatenates them into a single file, and saves it as "total.stl" in the specified directory.

import FreeCAD
import Mesh
import MeshPart
import os
import shutil
import time  # Import the time module

# Function to create and export all bodies (even if invalid)
def create_and_export_all_bodies(doc, export_directory):
    print(f"Creating and exporting all bodies to {export_directory}...")
    if not os.path.exists(export_directory):
        os.makedirs(export_directory)
        print(f"Created directory {export_directory}.")

    for obj in doc.Objects:
        print(f"Processing object: {obj.Label}")
        if "_Body" in obj.Label:
            # Attempt to create a mesh from the shape
            mesh_name = obj.Label.rstrip("_Body") + "Mesh"
            mesh_feature = doc.addObject("Mesh::Feature", mesh_name)
            shape = obj.Shape
            mesh_feature.Mesh = MeshPart.meshFromShape(
                Shape=shape, 
                LinearDeflection=0.1, 
                AngularDeflection=0.523599, 
                Relative=False
            )
            
            doc.recompute()
            print(f"Mesh created for '{obj.Label}'.")

            # Export the mesh
            export_path = os.path.join(export_directory, mesh_feature.Label + ".ast")  # Change extension if needed
            
            print(f"Exporting '{mesh_feature.Label}' to {export_path}...")

            Mesh.export([mesh_feature], export_path)
            
            print(f"Exported '{mesh_feature.Label}' to {export_path}.")
            time.sleep(1)  # Wait for a short period to ensure the file is written
            print(f"Exported '{mesh_feature.Label}' to {export_path}.")
            
            # Modify the exported file
            modify_exported_file(export_path)
            time.sleep(0.1)  # Wait for a short period to ensure the file is written               
    doc.recompute()
    print("Mesh bodies creation and export process completed.")

def modify_exported_file(file_path):
    print(f"Modifying exported file: {file_path}")
    try:
        with open(file_path, 'r') as file:
            lines = file.readlines()
        
        if lines:
            base_name = file_path.split("/")[-1].split(".")[0].replace("Mesh", "")
            lines[0] = "solid " + base_name + "\n"
            lines[-1] = "endsolid " + base_name + "\n"
        
        with open(file_path, 'w') as file:
            file.writelines(lines)
        
        base, _ = os.path.splitext(file_path)
        newfileName = base + ".stl"
        os.rename(file_path, newfileName)
        
        print(f"Modified first and last lines of {file_path}.")
        print(f"File renamed to {newfileName}.")
    
    except Exception as e:
        print(f"Could not modify {file_path}: {str(e)}")

def concatenate_stl_files(directory, output_file):
    print(f"Concatenating STL files in {directory} to {output_file}...")
    stl_files = [f for f in os.listdir(directory) if f.endswith('.stl')]
    if not stl_files:
        print("No .stl files found in the directory.")
        return

    with open(output_file, 'w') as outfile:
        for stl_file in stl_files:
            file_path = os.path.join(directory, stl_file)
            print(f"Adding {file_path}...")
            with open(file_path, 'r') as infile:
                lines = infile.readlines()
                time.sleep(0.1)  # Wait for a short period to ensure the file is read
                # Skip the first line of each file, which is the "solid" line
                outfile.writelines(lines)  # Skip the first line
    print(f"Combined .stl files saved as {output_file}.")
    
def export(doc, export_dir):
    print(f"Starting export process to {export_dir}...")
    create_and_export_all_bodies(doc, export_dir)
    # Wait for a short period to ensure all files are written
    time.sleep(0.1)  # Adjust the sleep time as necessary
    concatenate_stl_files(export_dir, os.path.join(export_dir, "total.stl"))
    # Remove mesh Features
    for obj in doc.Objects:
        if "Mesh" in obj.Label:
            print(f"Removing mesh feature: {obj.Label}")
            doc.removeObject(obj.Name)
            doc.clearDocument
            print("Document cleared.")
            doc.clearUndos
            print("Undos cleared.")
    print("Export process completed.")