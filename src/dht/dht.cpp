#include "dht.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include "person.capnp.h"

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