# NDArray-CPP

I use this repository to learn more about C++ by playing around and reimplementing some stuff that is provided by Eigen and OpenCV.

## Usage

1. Configure Build Configurations, I use Ninja Multi-Config build, with `Release`, `Debug` and `Asan` configurations.. The default preset handles all of this.

    ```bash
    cmake --presets default
    ```

    This also creates a `compile_commands.json` file in the `./build/` directory. You may want to create a symlink to that file in your source directory so that your IDE and/or other tools can find it.

2. Build your preferred configuration as

    ```bash
    cmake --build --preset <Release|Debug|Asan>
    ```

3. Run the binary as

    ```bash
    ./build/<Release|Debug|Asan>/ndarray_cpp
    ```

4. You can also build all three configurations by simply selecting the workflow `All` as

    ```bash
    cmake --workflow All
    ```

5. You can clean the builds by
    ```bash
    cmake --build --preset Clean
    ```
