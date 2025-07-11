#include <SFML/Network.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <chrono>


sf::TcpListener waiter(sf::TcpSocket& other, int port)
{
    sf::TcpListener result;
    if (result.listen(port) != sf::Socket::Status::Done)
    {
        std::cout << "Can't bind waiter socket to port. Debug please." << std::endl;
    }
    else
        std::cout << "Listener binded to port " << port << std::endl;
    if (result.accept(other) != sf::Socket::Status::Done)
    {
        std::cout << "Cant't accept connection. Please Debug." << std::endl;
    }
    else
    {
        std::cout << "connected by " << other.getRemoteAddress().value() << std::endl;
    }
    return result;
}
sf::TcpSocket maker(sf::TcpSocket& socket, sf::IpAddress ip, int port)
{
    sf::TcpSocket res;
    if (res.connect(ip, port) != sf::Socket::Status::Done)
    {
        std::cout << "Can't connect. Please Debug." << std::endl;
    }
    else
        std::cout << "Succesfully connected." << std::endl;
    return res;
}

std::atomic<bool> running(true);
std::mutex mtx;
std::queue<std::string> inputQueue;
void inputThread()
{
    std::string input;
    while (running && std::getline(std::cin, input, '\n'))
    {
        auto lock = std::unique_lock<std::mutex>(mtx);
        if (input == "exit")
        {
            running = false;
            break;
        }
        inputQueue.push(input);
        lock.unlock();
    }
    
}

int main()
{
    sf::TcpSocket person;
    sf::TcpListener listener;
    int port = 53000;
    std::cout << "Enter port to connection(if empty, then it will be 53000): ";
    if (!(std::cin >> port))
        port = 53000;
    std::cout << std::endl;
    int choice = 0;
    while (choice == 0)
    {
        std::cout << "Enter 1, if you want to wait for connection, 2 for connect to someone: ";
        std::cin >> choice;
        switch (choice)
        {
        case 1:
        {
            listener = waiter(person, port);
            break;
        }
        case 2:
        {
            std::optional<sf::IpAddress> ip;
            while (!ip)
            {
                std::cout << "Enter ip: ";
                std::string sip;
                std::cin.ignore();
                std::getline(std::cin, sip);
                std::cin.clear();
                std::cout << "\n";
                ip = sf::IpAddress::resolve(sip);
                if (!ip)
                {
                    std::cout << "IpAdress is invalid. ";
                }
            }
            person = maker(person, ip.value(), port);
            break;
        }
        default:
        {
            std::cout << "Please enter 1 or 2." << std::endl;
            choice = 0;
            break;
        }
        }
    }
    
    
    sf::Packet packet;
    person.setBlocking(false);
    std::thread t(inputThread);
    while (running)
    {
        if (person.receive(packet) == sf::Socket::Status::Done)
        {
            std::string msg;
            packet >> msg;
            std::cout << person.getRemoteAddress().value() << ": " <<  msg << std::endl;
        }
        if (!inputQueue.empty())
        {
            auto lock = std::unique_lock<std::mutex>(mtx);
            std::string input = inputQueue.front();
            packet.clear();
            packet << input;
            inputQueue.pop();
            if (person.send(packet) != sf::Socket::Status::Done)
            {
                std::cout << "Cant send data" << std::endl;
            }
            lock.unlock();
        }
    }
    t.join();
    return 0;
}