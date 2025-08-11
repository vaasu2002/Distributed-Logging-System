#include <iostream>
#include <string>
#include <set>
#include <sstream>
#include <algorithm>
#include <atomic>
#include <thread>
#include "LogPriorityQueue.h"
#include "LogDLQueue.h"
#include "MQReaderThreadWorker.h"
#include "ThreadWorkerPool.h"
#include "LogWriterThreadWorker.h"
#include "ConfigEditor.h"

void printVector(const std::vector<std::string>& vec, const std::string& label) {
	std::cout << label << ":\n";
	for (const auto& item : vec) {
		std::cout << "  - " << item << "\n";
	}
}

void editConfigMenu() {
	ConfigEditor editor("config.json");

	int appId;
	std::cout << "\nEnter App ID (e.g., 5445): ";
	std::cin >> appId;

	std::vector<std::string> currentFilters, currentAppenders;
	if (editor.getAppConfig(appId, currentFilters, currentAppenders)) {
		std::cout << "\nCurrent Configuration for app_" << appId << ":\n";
		printVector(currentFilters, "Filters");
		printVector(currentAppenders, "Appenders");
	}
	else {
		std::cout << "\nNo existing configuration found for app_" << appId << ".\n";
	}

	std::vector<std::string> filterOptions = { "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };

	int filterChoice;
	std::cout << "\nChoose one filter level:\n";
	for (size_t i = 0; i < filterOptions.size(); ++i) {
		std::cout << i + 1 << ": " << filterOptions[i] << "\n";
	}
	std::cout << "Enter filter number: ";
	std::cin >> filterChoice;
	std::cin.ignore(); // Clear newline

	if (filterChoice < 1 || filterChoice > static_cast<int>(filterOptions.size())) {
		std::cout << "Invalid filter choice.\n";
		return;
	}

	// Keep appenders as they are
	std::vector<std::string> newFilters = { filterOptions[filterChoice - 1] };
	std::vector<std::string> unchangedAppenders = currentAppenders;

	if (editor.updateAppConfig(appId, newFilters, unchangedAppenders)) {
		std::cout << "\nSuccessfully updated app_" << appId << " configuration.\n";
	}
	else {
		std::cerr << "\nFailed to update configuration.\n";
	}
}

int main() {
	const int NUM_READERS = 4;
	const size_t MAX_DEAD_LETTER_MB = 10;
	std::atomic<bool> running{ true };

	auto* priorityQueue = new LogPriorityQueue();
	auto* deadLetterQueue = new LogDLQueue(MAX_DEAD_LETTER_MB);

	// Start background log threads
	ThreadWorkerPool readerPool(NUM_READERS, priorityQueue, deadLetterQueue);
	LogWriterThread writer(priorityQueue, deadLetterQueue);

	readerPool.startAll();
	writer.start();

	// Console menu loop
	while (running) {
		std::cout << "\n=== Console Menu ===\n";
		std::cout << "1. Edit Config (Filter Only)\n";
		std::cout << "2. Exit\n";
		std::cout << "Choose option: ";

		int choice;
		if (!(std::cin >> choice)) {
			std::cin.clear();
			std::cin.ignore(10000, '\n');
			continue;
		}

		std::cin.ignore(); // clear newline

		if (choice == 1) {
			editConfigMenu();
		}
		else if (choice == 2) {
			running = false;
		}
		else {
			std::cout << "Invalid option.\n";
		}
	}

	// Stop threads cleanly
	readerPool.stopAll();
	writer.stop();

	std::cout << "\nShutdown complete.\n";
	return 0;
}
