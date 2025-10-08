#include <iostream>

class Base_Animal
{
public:
	Base_Animal() {
		std::cout << "Base Animal Constructor" << std::endl;
	};
	virtual ~Base_Animal() {
		std::cout << "Base Animal Deconstructor" << std::endl;
	};
	virtual void makeSound() {
		std::cout << "Unknow animal sound!" << std::endl;
	};
	virtual void func() = 0;
};

class Dog : public Base_Animal
{
public:
	Dog(){
		std::cout << "Dog Constructor" << std::endl;
	};
	~Dog() override{
		std::cout << "Dog Deconstructor" << std::endl;
	};
	void makeSound() override {
		std::cout << "This is Dog sound!" << std::endl;
	};
	void func() override {};
	void sayHello() {
		std::cout << "Hello. I am Dog" << std::endl;
	};
};

class Cat : public Base_Animal
{
public:
	// void makeSound() override {
	// 	std::cout << "This is Cat sound!" << std::endl;
	// };
	void func() override {};
	void sayHello() {
		std::cout << "Hello. I am Cat" << std::endl;
	};
};


int main(void)
{
	Base_Animal* animal1 = new Dog();
	animal1->makeSound();
	delete animal1;

	// Cat* animal2 = new Cat();
	// animal2->makeSound();
	// delete animal2;

	return 0;
}