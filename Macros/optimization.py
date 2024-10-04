import subprocess
from PySide2.QtCore import QThread, QObject, Slot
import FreeCAD
import os
import time
import ExportTotalStl
import logging




# Header for all paths
BASE_PATH = os.getcwd()
DAKOTA_INPUT_FILE = os.path.join(BASE_PATH, 'dakota_case.in')
SYSTEM_PATH = os.path.join(BASE_PATH,'casebase','system')
TRISURFACE_PATH = os.path.join(BASE_PATH, 'constant', 'triSurface')
LOG_FILE_PATH = os.path.join(BASE_PATH, 'process_log.log')
SIMULATOR_PATH = os.path.join(BASE_PATH,'simulator_script')
TEMPLATE_DIR =  os.path.join(BASE_PATH,'templatedir','*')
WORK_DIR = os.path.join(BASE_PATH,'workdir')


class DakotaRunner(QThread):
    
    def run(self):
        bashCommand = f"dakota -i {DAKOTA_INPUT_FILE} -o dakota.out"
        process = subprocess.Popen(bashCommand, shell=True, executable='/bin/bash', stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        
        try:
            process.wait()
        except KeyboardInterrupt:
            process.kill()
            print("Process was killed due to interruption.")


class SketchProcessor(QThread):

    def __init__(self, doc, base_path):
        super().__init__()
        self.doc = doc
        self.base_path = base_path
        self.counter = 1
        self.running = True
        
    @Slot() 
    def run(self):
        while self.running:
            folder_path = os.path.join(self.base_path, f"workdir.{self.counter}")
            if os.path.isdir(folder_path):
                csv_path = os.path.join(folder_path, "value.csv")
                if os.path.isfile(csv_path):
                    try:
                        # Process sketches from CSV
                        self.process_sketches(csv_path)
                        # Export STL after processing sketches
                        self.export_stl()
                        print(f"Sketches processed for {csv_path}")
                    except Exception as e:
                        print(f"Error processing sketches: {e}")
                    self.counter += 1
                else:
                    print(f"File {csv_path} does not exist")

            else:
                logging.warning(f"Folder {folder_path} does not exist")
            time.sleep(1)  # Delay before checking the next folder

    def process_sketches(self, csv_path):
        try:
            with open(csv_path, 'r') as file:
                data = file.read().replace('\n', '').split(',')
            
            if self.doc is None:
                logging.error("No active document found.")
                return

            changed = False
            for obj in self.doc.Objects:
                if obj.TypeId == "Sketcher::SketchObject":
                    for constraint in obj.Constraints:
                        constraint_index = self.get_constraint_index(constraint.Name)
                        if constraint_index is not None and 0 <= constraint_index < len(data):
                            data_value = data[constraint_index]
                            if obj.getDatum(constraint.Name) != data_value:
                                print(f"Setting {constraint.Name} to {data_value}")
                                obj.setDatum(constraint.Name, FreeCAD.Units.Quantity(data_value + " mm"))
                                time.sleep(1)  # Wait for a short period to ensure the value is set
                                print(f"Set {constraint.Name} to {data_value}") 
                                time.sleep(1)  # Wait for a short period to ensure the value is set
                                changed = True
            
            if changed:
                print("Sketches changed.")
                self.doc.recompute()  # Recompute document only if sketches changed
                print("Sketches recomputed.")
        except Exception as e:
            print(f"Error processing sketches: {e}")

    def get_constraint_index(self, name):
        try:
            return int(name.strip("param")) - 1  # Extract index from constraint name
        except ValueError:
            print(f"Error getting constraint index for {name}")
            return None

    def export_stl(self):
        try:
            trisurface_path = os.path.join(self.base_path, f"workdir.{self.counter}", 'casebase', 'constant', 'triSurface')
            os.makedirs(trisurface_path, exist_ok=True)
            ExportTotalStl.export(self.doc, trisurface_path)
        except Exception as e:
            print(f"Error exporting STL: {e}")

    def stop(self):
        self.running = False
        self.wait()  # Ensure the thread finishes before stopping

class Controller(QObject):
    
    def __init__(self):
        super().__init__()

        self.dakota_runner = DakotaRunner()
        self.sketch_processor = None
    
    @Slot() 
    def patchAreas(self):
        doc = FreeCAD.ActiveDocument
        if os.path.exists(os.path.join(SYSTEM_PATH, 'outletAreas')):
            os.remove(os.path.join(SYSTEM_PATH, 'outletAreas'))
        if doc is not None:
            for obj in doc.Objects:
                if obj.TypeId == 'PartDesign::SubShapeBinder':
                    area = obj.Shape.Area
                    print("Writing area to file")
                    with open(os.path.join(SYSTEM_PATH, 'outletAreas'), 'a') as f:
                        f.write(str(obj.Label).strip("_Body") + " " + str(area/1e6) + ';\n')
        else:
            logging.error("No active FreeCAD document found")


    def patchesNames(self):
        doc = FreeCAD.ActiveDocument
        if os.path.exists(os.path.join(SYSTEM_PATH, 'patches')):
            os.remove(os.path.join(SYSTEM_PATH, 'patches'))
        if doc is not None:
            for obj in doc.Objects:
                if obj.TypeId == 'PartDesign::SubShapeBinder':
                    print("Writing patch names to file")
                    with open(os.path.join(SYSTEM_PATH, 'patches'), 'a') as f:
                        f.write(f"{obj.Label.strip('_Body')}\n{{\nname {obj.Label.strip('_Body')} ;\n}}\n")
        else:
            logging.error("No active FreeCAD document found")


    def outletPatches(self):
        doc = FreeCAD.ActiveDocument
        if os.path.exists(os.path.join(SYSTEM_PATH, 'outletPatches')):
            os.remove(os.path.join(SYSTEM_PATH, 'outletPatches'))
        IS_count = 0
        ES_count = 0
        if doc is not None:
            for obj in doc.Objects:
                if obj.TypeId == 'PartDesign::SubShapeBinder':
                    if 'IS' in obj.Label:
                        IS_count += 1
                    if 'ES' in obj.Label:
                        ES_count += 1
            print("Writing outlet patches to file")
            with open(os.path.join(SYSTEM_PATH, 'outletPatches'), 'a') as f:
                f.write(f"nIS {IS_count};\n")
                f.write(f"nES {ES_count};\n")
        else:
            logging.error("No active FreeCAD document found")
                        
    def centerofmass(self):
        doc = FreeCAD.ActiveDocument
        if doc is not None:
            center_of_mass_file = os.path.join(SYSTEM_PATH, 'centerofmass')
            if os.path.exists(center_of_mass_file):
                os.remove(center_of_mass_file)
            for obj in doc.Objects:
                if obj.TypeId == 'PartDesign::Body':
                    x, y, z = obj.Shape.CenterOfMass
                    if os.path.exists(center_of_mass_file):
                        os.remove(center_of_mass_file)
                    with open(center_of_mass_file, 'a') as f:
                        f.write(f"({x:.2f} {y:.2f} {z:.2f});\n")
        else:
            print("No active FreeCAD document found")
            
    def dakota_input(self):
        try:
            # Read the template file
            with open(os.path.join(BASE_PATH, 'templatedir', 'dakota_case.tmp'), 'r') as file:
                data = file.read()

            # Write the original content to the Dakota input file
            with open(DAKOTA_INPUT_FILE, 'w') as file:
                file.write(data)

            # Replace placeholders with actual paths
            data = data.replace('{x1}', f"'{SIMULATOR_PATH}'")
            data = data.replace('{x2}', f"'{TEMPLATE_DIR}'")
            data = data.replace('{x3}', f"'{WORK_DIR}'")

            # Write the modified content to the Dakota input file again
            with open(DAKOTA_INPUT_FILE, 'w') as file:
                file.write(data)
        except Exception as e:
            print(f"Error creating Dakota input file: {e}")
            
    def boundingBox(self):
        doc = FreeCAD.ActiveDocument
        if doc is not None:
            bounding_box_file = os.path.join(SYSTEM_PATH, 'boundingBox')
            if os.path.exists(bounding_box_file):
                os.remove(bounding_box_file)
            for obj in doc.Objects:
                if obj.TypeId == 'PartDesign::Body':
                    Xmax, Xmin = obj.Shape.BoundBox.XMax, obj.Shape.BoundBox.XMin
                    Ymax, Ymin = obj.Shape.BoundBox.YMax, obj.Shape.BoundBox.YMin
                    Zmax, Zmin = obj.Shape.BoundBox.ZMax, obj.Shape.BoundBox.ZMin
                    if os.path.exists(bounding_box_file):
                        os.remove(bounding_box_file)
                    print("Writing Bounding box to file")
                    with open(bounding_box_file, 'a') as f:
                        f.write(f"xmin {Xmin - 1:.2f};\n")
                        f.write(f"xmax {Xmax + 1:.2f};\n")
                        f.write(f"ymin {Ymin - 1:.2f};\n")
                        f.write(f"ymax {Ymax + 1:.2f};\n")
                        f.write(f"zmin {Zmin - 1:.2f};\n")
                        f.write(f"zmax {Zmax + 1:.2f};\n")
        else:
            print("No active FreeCAD document found")

    def start(self):
        # Start Dakota in a separate thread
        self.dakota_runner.start()

        # Get the active FreeCAD document
        doc = FreeCAD.ActiveDocument
        if doc is not None:
            # Start processing sketches in a separate thread
            self.sketch_processor = SketchProcessor(doc, BASE_PATH)
            self.sketch_processor.start()
        else:
           print("No active FreeCAD document found")

if __name__ == "__main__":
    
    # Create and start the Controller
    controller = Controller()
    
    controller.dakota_input()
    
    controller.outletPatches()
    
    # Create the areas file
    controller.patchAreas()
    
    controller.patchesNames()

    # Create the bounding box file
    controller.boundingBox()

    controller.centerofmass()

    # Start the process
    controller.start()