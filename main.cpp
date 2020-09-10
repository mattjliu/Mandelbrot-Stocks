#include <iostream>
#include <array>
#include <random>
#include <omp.h>
#include <thread>
#include <time.h>  

#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"

const auto processor_count = std::thread::hardware_concurrency();

int main()
{
    const int WIDTH = 1920, HEIGHT = 400;
    const int screenWidth = 1920, screenHeight = 1080;
    sf::RenderWindow window(sf::VideoMode(screenWidth, screenHeight), "Mandelbrot Stocks", sf::Style::Default);
    window.setFramerateLimit(60);

    std::vector<std::pair<double, double>> initial_points{
        std::make_pair((double)0, (double)0),
        std::make_pair((double)3 / 9, (double)2 / 3),
        std::make_pair((double)6 / 9, (double)1 / 3),
        std::make_pair((double)1, (double)1)
    };

    const int mult_points = initial_points.size() - 1;

    std::vector<std::pair<double, double>> deltas;
    deltas.reserve(mult_points);
    for (std::size_t i = 0; i < initial_points.size() - 1; i++)
    {
        deltas.push_back(
            std::make_pair(
                initial_points[i + 1].first - initial_points[i].first,
                initial_points[i + 1].second - initial_points[i].second
            )
        );
    }


    std::vector<std::pair<double, double>> new_points;
    const int limit = 14;
    for (int i = 0; i < limit; i++)
    {
        std::cout << "Iteration " << i+1 << std::endl;
        std::vector<std::pair<double, double>> new_points((initial_points.size()-1) * mult_points + 1, std::pair<double, double>());

        #pragma omp parallel for firstprivate(deltas) num_threads(processor_count)
        for (int i = 0; i < initial_points.size() - 1; i++)
        {
            std::srand(time(0) + omp_get_thread_num());
            double xscale = initial_points[i + 1].first - initial_points[i].first;
            double yscale = initial_points[i + 1].second - initial_points[i].second;
            std::random_shuffle(deltas.begin(), deltas.end());

            new_points[i * mult_points] = initial_points[i];

            for (int j = 0; j < mult_points - 1; j++)
            {
                new_points[i * mult_points + j + 1] = std::make_pair(
                    new_points[i * mult_points + j].first + xscale * deltas[j].first,
                    new_points[i * mult_points + j].second + yscale * deltas[j].second
                );
            }     
        }
        new_points[(initial_points.size()-1) * mult_points] = initial_points[initial_points.size() - 1];
        initial_points = new_points;
    }

    new_points.clear();

    std::cout << "Creating vertex array..." << std::endl;
    sf::VertexArray display_lines(sf::LineStrip, initial_points.size());
    #pragma omp parallel for shared(display_lines) num_threads(processor_count)
    for (int i = 0; i < initial_points.size(); i++)
    {
        display_lines[i].position = sf::Vector2f(initial_points[i].first * WIDTH, screenHeight * 0.75 - initial_points[i].second * HEIGHT);
        display_lines[i].color = sf::Color::White;
    }

    initial_points.clear();

    sf::View view(sf::Vector2f(screenWidth / 2.f, screenHeight / 2), sf::Vector2f(screenWidth, screenHeight));
    while (window.isOpen())
    {
        sf::Event event;

        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed)
            {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                    view.zoom(1.f / 1.01f);
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                    view.zoom(1.01f);
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
                    view.move(sf::Vector2f(0.f, -1.f));
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
                    view.move(sf::Vector2f(-1.f, 0.f));
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
                    view.move(sf::Vector2f(0.f, 1.f));
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
                    view.move(sf::Vector2f(1.f, 0.f));
            }
        }

        window.setView(view);
        window.clear();
        window.draw(display_lines);

        window.display();
    }

    return 0;
}