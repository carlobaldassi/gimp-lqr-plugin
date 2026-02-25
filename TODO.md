

Compilation Problems & GIMP 3.0 Migration Requirements

1. TRIVIAL CHANGES (Simple API renames/replacements)

GTK Widget Deprecations:

- GtkTable → GtkGrid: altsizeentry.h:49, 67 - Replace GtkTable parent class with GtkGrid
- GtkTableClass → GtkGridClass: Replace in widget inheritance

GIMP Type Deprecations:

- GimpUnit: altsizeentry.h:55 - Type definition changed in GIMP 3.0
- GimpRGB: main.c:130,137,144,151 - Color type structure changed

Autotools Warnings:

- AM_CONFIG_HEADER → AC_CONFIG_HEADERS: configure.ac:27
- AM_PROG_CC_STDC → AC_PROG_CC: configure.ac:33
- AC_HEADER_STDC: Obsolete, needs removal
- AC_PROG_INTLTOOL: Deprecated macro

2. SIGNIFICANT CHANGES (Require substantial rework)

Plugin Registration System:

- GimpParamDef → GimpProcedure: main.c:165-194 - Complete rewrite needed
- GIMP_PDB_ constants*: All parameter type constants changed
- gimp_install_procedure(): Replaced with new procedure API
- Plugin structure: GimpPlugInInfo replaced with GimpPlugIn class

Plugin Run Function:

- Function signature: main.c:54-57 - Parameters completely changed
- GimpParam arrays: Replaced with GimpProcedureConfig
- Return value handling: New procedure return system

GIMP Drawing API:

- Layer/Image handling: Many functions renamed or restructured
- Drawable operations: API significantly changed
- Progress callbacks: New progress reporting system

3. DETAILED MIGRATION REQUIREMENTS

High Priority (Blocking compilation):

1. Plugin Entry Point (main.c)
   - Replace MAIN() macro with new plugin class system
   - Rewrite query() and run() functions entirely
   - Convert parameter definitions to new procedure API
2. Widget System (altsizeentry.h/c)
   - Port from GtkTable to GtkGrid
   - Update widget creation and layout code
   - Fix GimpUnit type usage
3. GIMP API Calls (All source files)
   - Replace deprecated image/layer functions
   - Update progress reporting
   - Fix color/unit type usage

Medium Priority:

4. UI Interface (interface*.c)
   - Update GTK widget creation patterns
   - Fix deprecated GIMP UI functions
   - Update dialog construction
5. Rendering Pipeline (render.c)
   - Update GIMP drawing API calls
   - Fix pixel buffer access patterns
   - Update layer creation/manipulation

Low Priority:

6. Build System (configure.ac)
   - Update autotools macros
   - Fix deprecated configuration patterns

4. REQUIRED GIMP 3.0 API KNOWLEDGE

- New procedure system: gimp_image_procedure_new()
- GimpProcedureConfig: For parameter handling
- GimpResource: For units/colors
- New widget APIs: GTK4-compatible widgets
- GeglBuffer: For pixel data access
