## How to Build and Run the Project  

### 1. Open the Project in Visual Studio Code (or other preferred IDEs)
- Ensure you have **VS Code** installed.  
- Open the project folder in **VS Code**.  

### 2. Edit the Configuration  
- Open `input.txt`.  
- Modify the values for the following parameters as needed:  
  ```
  n, t, h, d, t1, t2
  ```  

### 3. Build and Compile the Project  

#### Option 1: Using VS Code's Run Button  
- Open `main.cpp`.  
- Click the **Run Code** button in **VS Code**.  

#### Option 2: Using the Terminal  
- Open the terminal in **VS Code**.  
- Run the following command to compile the program:  
  ```sh
  g++ main.cpp -o main -std=c++20
  ```  
- Execute the compiled program:  
  ```sh
  ./main
  
