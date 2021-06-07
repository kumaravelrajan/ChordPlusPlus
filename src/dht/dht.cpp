#include "dht.h"
#include <util.h>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include "person.capnp.h"

// This is just required for the quick demo.
#ifdef _MSC_VER

#include <io.h>
#include <fcntl.h>

#endif

void dht::Dht::mainLoop()
{
    /*
     * This is where the dht will fix fingers, store and retrieve data, etc.
     * This needs to be thread-safe, and needs to be able to exit at any time.
     * (No blocking function calls in here)
     */

    using namespace std::chrono_literals;

    std::cout << "[DHT] Main Loop Entered" << std::endl;

#ifdef _MSC_VER
    if (int p[2]; _pipe(p, 1024, _O_BINARY) >= 0) {
#else
        if (int p[2]; pipe(p) >= 0) {
#endif
        std::this_thread::sleep_for(1s);
        dht::writeMessage(p[1]);
        std::this_thread::sleep_for(1s);
        dht::printMessage(p[0]);
        std::this_thread::sleep_for(1s);
    }

    std::cout << "[DHT] Exiting Main Loop" << std::endl;
}

void dht::Dht::setApi(std::unique_ptr<api::Api> api)
{
    // Destroy old api:
    m_api = nullptr;
    // Move in new api:
    m_api = std::move(api);

    // Set Request handlers:
    m_api->on<util::constants::DHT_PUT>(
        [this](const api::Message_KEY_VALUE &m, std::atomic_bool &cancelled) {
            return onDhtPut(m, cancelled);
        });
    m_api->on<util::constants::DHT_GET>(
        [this](const api::Message_KEY &m, std::atomic_bool &cancelled) {
            return onDhtGet(m, cancelled);
        });
}

std::vector<std::byte> dht::Dht::onDhtPut(const api::Message_KEY_VALUE &m, std::atomic_bool &cancelled)
{
    return {};
}

std::vector<std::byte> dht::Dht::onDhtGet(const api::Message_KEY &m, std::atomic_bool &cancelled)
{
    return {};
}


void dht::writeMessage(int fd)
{
    capnp::MallocMessageBuilder message;

    AddressBook::Builder addressBook = message.initRoot<AddressBook>();
    ::capnp::List<Person>::Builder people = addressBook.initPeople(2);

    Person::Builder alice = people[0];
    alice.setName("Alice");
    alice.setEmail("alice@example.com");
    // Type shown for explanation purposes; normally you'd use auto.
    ::capnp::List<Person::PhoneNumber>::Builder alicePhones =
        alice.initPhones(1);
    alicePhones[0].setNumber("555-1212");
    alicePhones[0].setType(Person::PhoneNumber::Type::MOBILE);

    Person::Builder bob = people[1];
    bob.setName("Bob");
    bob.setEmail("bob@example.com");
    auto bobPhones = bob.initPhones(2);
    bobPhones[0].setNumber("555-4567");
    bobPhones[0].setType(Person::PhoneNumber::Type::HOME);
    bobPhones[1].setNumber("555-7654");
    bobPhones[1].setType(Person::PhoneNumber::Type::WORK);

    capnp::writePackedMessageToFd(fd, message);
}

void dht::printMessage(int fd)
{
    capnp::PackedFdMessageReader message(fd);

    AddressBook::Reader addressBook = message.getRoot<AddressBook>();

    for (Person::Reader person : addressBook.getPeople()) {
        std::cout << person.getName().cStr() << ": "
                  << person.getEmail().cStr() << std::endl;
        for (Person::PhoneNumber::Reader phone: person.getPhones()) {
            const char *typeName = "UNKNOWN";
            switch (phone.getType()) {
                case Person::PhoneNumber::Type::MOBILE:
                    typeName = "mobile";
                    break;
                case Person::PhoneNumber::Type::HOME:
                    typeName = "home";
                    break;
                case Person::PhoneNumber::Type::WORK:
                    typeName = "work";
                    break;
            }
            std::cout << "  " << typeName << " phone: "
                      << phone.getNumber().cStr() << std::endl;
        }
    }
}