#include <mpi.h>                 
#include <iostream>             
#include <fstream>               
#include <vector>                
#include <chrono>                

// Функція перевірки
bool has_exactly_4_divisors(int x) {
    int count = 0;
    for (int i = 1; i * i <= x; ++i) {             
        if (x % i == 0) {
            count += (i * i == x) ? 1 : 2;         
            if (count > 4) return false;           
        }
    }
    return count == 4;                            
}

int main(int argc, char** argv) {
    if (argc < 2) {                                
        std::cerr << "Використання: mpiexec -n <N> ./filter_mpi_sendrecv numbers.bin\n";
        return 1;
    }

    MPI_Init(&argc, &argv);                         //Ініціалізуємо MPI
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);           //номер поточного процесу
    MPI_Comm_size(MPI_COMM_WORLD, &size);           //загальна кількість процесів

    std::vector<int> local_data;                    //Вектор для зберігання локального шматка даних
    unsigned long long chunk_size = 0;              //Розмір блоку (кількість чисел для кожного процесу)

    double elapsed = 0.0;                           //Змінна для зберігання часу виконання

    if (rank == 0) {                                //Основна логіка тільки на процесі 0 (master)
        auto start = std::chrono::high_resolution_clock::now();   

        std::ifstream in(argv[1], std::ios::binary | std::ios::ate);  // Відкриваємо файл на читання у двійковому режимі
        if (!in) {
            std::cerr << "Не вдалося відкрити файл\n";
            MPI_Abort(MPI_COMM_WORLD, 1);           //аварійно завершуємо всі процеси
        }

        size_t filesize = in.tellg();               
        size_t total_numbers = filesize / sizeof(int); //Розраховуємо кількість чисел
        in.seekg(0);                                 //Повертаємося на початок файлу

        std::vector<int> all_data(total_numbers);    //Вектор для зберігання всіх чисел з файлу
        in.read(reinterpret_cast<char*>(all_data.data()), filesize);  
        in.close();                                  

        chunk_size = total_numbers / size;           //Розмір блоку для кожного процесу
        size_t remainder = total_numbers % size;     //Що залишилось після поділу на рівні частини

        //Відправляємо кожному процесу його блок даних
        for (int i = 1; i < size; ++i) {
            unsigned long long send_count = (i == size - 1) ? chunk_size + remainder : chunk_size;
            MPI_Send(&send_count, 1, MPI_UNSIGNED_LONG_LONG, i, 0, MPI_COMM_WORLD);  // Відправляємо розмір
            MPI_Send(all_data.data() + i * chunk_size, static_cast<int>(send_count), MPI_INT, i, 0, MPI_COMM_WORLD);  // Відправляємо дані
        }

        //Обробляємо перший блок даних 
        local_data.assign(all_data.begin(), all_data.begin() + chunk_size);

        int local_count = 0;
        for (int x : local_data)
            if (has_exactly_4_divisors(x))
                ++local_count;

        int global_count = local_count;              // Ініціалізуємо загальний лічильник

        //Збираємо результати з інших процесів
        for (int i = 1; i < size; ++i) {
            int recv_count = 0;
            MPI_Recv(&recv_count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Отримуємо від кожного результат
            global_count += recv_count;              // Додаємо до загального
        }

        auto end = std::chrono::high_resolution_clock::now();   //Кінець часу
        elapsed = std::chrono::duration<double>(end - start).count();

        std::cout << "Кількість чисел з 4 дільниками: " << global_count << "\n";
        std::cout << "Час виконання: " << elapsed << " сек\n";
    }
    else {
        //  Інші процеси отримують розмір і дані
        MPI_Recv(&chunk_size, 1, MPI_UNSIGNED_LONG_LONG, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //Отримуємо chunk_size
        local_data.resize(chunk_size);             // Розміщуємо пам’ять під свій блок
        MPI_Recv(local_data.data(), static_cast<int>(chunk_size), MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //Отримуємо дані

        int local_count = 0;
        for (int x : local_data)
            if (has_exactly_4_divisors(x))
                ++local_count;

        MPI_Send(&local_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); //ідправляємо результат назад процесу 0
    }

    MPI_Finalize();  // Завершення MPI-програми (після цього не можна викликати MPI-функції)
    return 0;        // Успішне завершення
}
//mpiexec -n 1 C:\Users\andre\source\repos\MPI4\x64\Debug\MPI4.exe C:\Users\andre\source\repos\MPI4\MPI4\numbers.bin
//mpiexec -n 2 C:\Users\andre\source\repos\MPI4\x64\Debug\MPI4.exe C:\Users\andre\source\repos\MPI4\MPI4\numbers.bin
//mpiexec -n 4 C:\Users\andre\source\repos\MPI4\x64\Debug\MPI4.exe C:\Users\andre\source\repos\MPI4\MPI4\numbers.bin
//mpiexec -n 8 C:\Users\andre\source\repos\MPI4\x64\Debug\MPI4.exe C:\Users\andre\source\repos\MPI4\MPI4\numbers.bin