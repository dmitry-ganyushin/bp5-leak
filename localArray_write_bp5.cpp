#include <iostream>
#include <vector>

#include <adios2.h>

int main(int argc, char *argv[])
{
    int rank = 0;
#if ADIOS2_USE_MPI
    int nproc = 1;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
#endif
    const int NSTEPS = 5;

    // generate different random numbers on each process,
    // but always the same sequence at each run
    srand(rank * 32767);

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif

    // v0 has the same size on every process at every step
    const size_t Nglobal = 40;
    std::vector<int8_t> v0(Nglobal);

    try
    {
        // Get io settings from the config file or
        // create one with default settings here
        adios2::IO io = adios.DeclareIO("Output");
        io.SetEngine("BP5");
        io.SetParameters({{"verbose", "4"}});

        /*
         * Define local array: type, name, local size
         * Global dimension and starting offset must be an empty vector
         * Here the size of the local array is the same on every process
         */
        adios2::Variable<int8_t> varV0 =
            io.DefineVariable<int8_t>("v0", {}, {}, {Nglobal});

        adios2::Engine writer = io.Open("localArray.bp5", adios2::Mode::Write);

        for (int step = 0; step < NSTEPS; step++)
        {
            writer.BeginStep();
            for (int block = 0 ; block < 500; block++){
                // v0
                for (size_t i = 0; i < Nglobal; i++)
                {
                    v0[i] = rank * 1.0 + step * 0.1;
                }
                writer.Put<int8_t>(varV0, v0.data(), adios2::Mode::Sync);
            }

            writer.EndStep();
        }

        writer.Close();
    }
    catch (std::invalid_argument &e)
    {
        if (rank == 0)
        {
            std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }
    catch (std::ios_base::failure &e)
    {
        if (rank == 0)
        {
            std::cout << "System exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }
    catch (std::exception &e)
    {
        if (rank == 0)
        {
            std::cout << "Exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return 0;
}
