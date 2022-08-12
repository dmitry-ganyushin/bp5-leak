#include <chrono>
#include <time.h>
#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <string>
#include <thread>
#include <vector>
#include <adios2.h>

void ReadVariable(const std::string &name, adios2::IO &io,
                  adios2::Engine &reader, size_t step)
{
    adios2::Variable<int8_t> variable = io.InquireVariable<int8_t>(name);

    if (variable)
    {
        auto blocksInfo = reader.BlocksInfo(variable, step);

        std::cout << "    " << name << " has " << blocksInfo.size()
                  << " blocks in step " << step << std::endl;

        // create a data vector for each block
        std::vector<int8_t> dataSet;
        dataSet.resize(blocksInfo.size());

        // schedule a read operation for each block separately
        int i = 0;

        for (auto &info : blocksInfo)
        {
            variable.SetBlockSelection(info.BlockID);
            int t0 = clock();
            reader.Get(variable, dataSet, adios2::Mode::Sync);
            printf("done in %f\n", 1.0 * (clock() - t0) / CLOCKS_PER_SEC);
        }
    }
    else
    {
        std::cout << "    Variable " << name << " not found in step " << step
                  << std::endl;
    }
}

int main(int argc, char *argv[])
{
    try
    {
        adios2::ADIOS adios;

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines
         * Inline uses single IO for write/read */
        adios2::IO io = adios.DeclareIO("Input");

        io.SetEngine("BP5");
        io.SetParameters({{"verbose", "4"}});

        adios2::Engine reader = io.Open("localArray.bp5", adios2::Mode::Read);

        while (true)
        {

            // Begin step
            adios2::StepStatus read_status =
                reader.BeginStep(adios2::StepMode::Read, 10.0f);
            if (read_status == adios2::StepStatus::NotReady)
            {
                // std::cout << "Stream not ready yet. Waiting...\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }
            else if (read_status != adios2::StepStatus::OK)
            {
                break;
            }

            size_t step = reader.CurrentStep();
            std::cout << "Process step " << step << ": " << std::endl;
            if (step == 0){
                ReadVariable("v0", io, reader, step);
            }
            reader.EndStep();
        }

        reader.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "IO System base failure exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank\n";
        std::cout << e.what() << "\n";
    }
}
