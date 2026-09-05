# amrexplot

`amrexplot` is intended as a tool that can be extended to load a saved state onto a list of plotfiles and generate images automatically without opening the GUI. Currently, it provides a programmatic way to extract and render slices from AMReX plotfiles.

## Features
- **Batch Processing**: Reads a list of plotfiles from a text file and processes them sequentially.
- **Aspect-Ratio Aware**: Automatically calculates the output image dimensions to maintain the physical aspect ratio of the dataset.
- **Automatic Organization**: Creates an `images/` directory to store all generated PNG files.
- **Clean Naming**: Saves images using the plotfile's base name (e.g., `plt0001` $\rightarrow$ `plt0001.png`).

## Prerequisites
The tool depends on the `amrexplorer` library which should be compiled first. See documentation for instructions.

## Usage

### Execution
Run the executable and provide a text file containing the paths to the plotfiles you wish to process:

```bash
make -j4
./amrexplot <filelist.txt>
```

### Input File Format (`filelist.txt`)
The input file should be a plain text file with one plotfile path per line:
```text
/path/to/data/plt00000
/path/to/data/plt00001
/path/to/data/plt00002
```

## Technical Details
- **Slice Orientation**: The current implementation generates a slice along the Z-axis (Normal Direction = 2).
- **Slice Position**: The slice is taken at the physical position $z = 0.0$.
- **Resolution**: The base resolution is set to 512 pixels along the longest axis.
- **Rendering**: Uses linear sampling and the finest available grid composition.

## Output
All results are saved in the `images/` folder relative to the execution directory.
