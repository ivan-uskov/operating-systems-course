# Лабораторная работа №5

- [Лабораторная работа №5](#лабораторная-работа-5)
  - [Задания](#задания)
    - [Требования](#требования)
    - [Задание 1 — Потокобезопасное множество — 50 баллов](#задание-1--потокобезопасное-множество--50-баллов)
    - [Задание 2 — Исполнитель задач в фоновом потоке — 100 баллов](#задание-2--исполнитель-задач-в-фоновом-потоке--100-баллов)
      - [Примеры](#примеры)
      - [Подсказки](#подсказки)
    - [Задание 3 — Пул потоков — 150 баллов](#задание-3--пул-потоков--150-баллов)
    - [Задание 4](#задание-4)
  - [Ссылки](#ссылки)

## Задания

- Для получения оценки "удовлетворительно" нужно набрать не менее X баллов.
- Для получения оценки "хорошо" нужно набрать не менее Y баллов.
- Для получения оценки "отлично" нужно набрать не менее Z баллов.

### Требования

Обязательно проверяйте успешность всех вызовов функций операционной системы и не оставляйте ошибки незамеченными.

Ваш код должен иметь уровень безопасности исключений не ниже базового.
Для этого разработайте (или возьмите готовую) RAII-обёртку, автоматизирующую
управление ресурсами операционной системы.

### Задание 1 — Потокобезопасное множество — 50 баллов

Ознакомьтесь с классами `std::mutex` и `std::shared_mutex`, а также обёртками
`std::lock_guard`, `std::unique_lock` и `std::shared_lock`.

Напишите программу, которая строит `unordered_set<uint64_t>` из простых чисел
в диапазоне от 0 до введённого пользователем числа.
Ограничения этой задачи не позволяют использовать решето Эратосфена,
поэтому приходится искать простые числа перебором делителей от 2 до $\sqrt N$.

Чтобы ускорить решение задачи, распределите поиск делителей между несколькими потоками.

Чтобы избежать состояния гонки потоков, напишите шаблонный класс `ThreadsafeSet`
(потокобезопасное множество), предоставляющий потокобезопасные операции
для доступа к множеству.

```c++
template <typename T, typename H = std::hash<T>, typename Comp = std::equal_to<T>>
class ThreadsafeSet
{
public:
    // Разместите здесь операции, которые можно безопасно вызывать из нескольких потоков
private:
    std::unordered_set<T, H, Comp> m_set;
};
```

Ваша программа должна измерить время работы алгоритма в одном потоке (использующего `unordered_set`),
а также время работы многопоточного алгоритма, использующего `ThreadsafeSet`.

Постройте диаграмму, отображающую время работы алгоритма для верхней границы в 1 млн, 10 млн, 100 млн
при количестве потоков от 1 до 16.

Проанализируйте полученные результаты и сделайте выводы.

### Задание 2 — Исполнитель задач в фоновом потоке — 100 баллов

Ознакомьтесь с классами `std::mutex` и `std::conditional_variable`.

Разработайте класс `BgThreadDispatcher`, который предоставляет возможность выполнять задачи в фоновом потоке.
Задачи представлены в виде `std::function<void()>`.
Задачи выполняются последовательно в порядке их поступления.

Исключение, выброшенное задачей, не должно препятствовать выполнению остальных задач.

Деструктор класса `BgThreadDispatcher` должен остановить фоновый поток и прекратить
выполнение задач в очереди.
Выполнение задач, которые не успели запуститься к моменту вызова деструктора, не гарантируется.

Для класса `BgThreadDispatcher` разработайте юнит-тесты.

Интерфейс класса представлен ниже:

```c++
class BgThreadDispatcher
{
public:
    using Task = std::function<void()>;

    // Копирование и присваивание объектов этого класса невозможно
    BgThreadDispatcher(const BgThreadDispatcher&) = delete;
    BgThreadDispatcher& operator=(const BgThreadDispatcher&) = delete;

    // Завершает работу фонового потока. 
    ~BgThreadDispatcher();

    // Отправляет задачу task на выполнение в фоновом потоке.
    // Фоновый поток последовательно выполняет задачи в порядке их поступления.
    // Вызов этого метода из фонового потока — неопределённое поведение.
    // Если BgThreadDispatcher остановлен, задача игнорируется.
    void Dispatch(Task task);

    // Блокирует работу текущего потока до окончания выполнения всех фоновых задач.
    // Вызов этого метода из фонового потока приводит к неопределённому поведению.
    // Если ранее был вызван метод Stop, то ожидание завершения не происходит.
    void Wait();

    // Сообщает фоновому потоку о необходимости остановить работу.
    // Задачи, которые не успели выполниться, игнорируются.
    void Stop();
};
```

#### Примеры

Эта программа выведет текст «1!»:

```c++
int main()
{
    using osync = std::osyncstream;
    BgThreadDispatcher dispatcher;
    dispatcher.Dispatch([]{ osync(std::cout) << "1"; }); // Выведет 1 в фоновом потоке
    dispatcher.Wait(); // Дождётся завершения фоновых задач 
    osync(std::cout) << "!"; // Выведет ! в основном потоке
}
```

Эта программа напечатает один из следующих текстов: «!123», «1!23» или «12!3»:

```c++
int main()
{
    using osync = std::osyncstream;
    BgThreadDispatcher dispatcher;
    dispatcher.Dispatch([]{ osync(std::cout) << "1"; }); // выведет 1 в фоновом потоке
    dispatcher.Dispatch([]{ osync(std::cout) << "2"; }); // Выведет 2 в фоновом потоке
    osync(std::cout) << "!"; // Выведет ! в основном потоке
    dispatcher.Wait(); // Дождётся печати 1 и 2
    dispatcher.Dispatch([]{ osync(std::cout) << "3"; }); // Выведет 3 в фоновом потоке
    dispatcher.Wait(); // Дождётся вывода 3
}
```

Эта программа напечатает один из следующих текстов: «!1», «1!» или «!»:

```c++
int main()
{
    using osync = std::osyncstream;
    BgThreadDispatcher dispatcher;
    dispatcher.Dispatch([]{ osync(std::cout) << "1"; }); // Выводим 1 в фоновом потоке
    osync(std::cout) << "!"; // Выводим ! в основном потоке
    // Вывод 1 не гарантируется, так как мы не вызвали Wait перед вызовом деструктора.
}
```

Следующая программа с большой вероятностью выведет «!1» или «1!». С очень низкой вероятностью выведет «!».

```c++
int main()
{
    using osync = std::osyncstream;
    BgThreadDispatcher dispatcher;
    dispatcher.Dispatch([]{ osync(std::cout) << "1"; }); // Выведет 1 в фоновом потоке
    osync(std::cout) << "!"; // Выведет ! в основном потоке
    // За 1 секунду фоновый поток успеет вывести 1
    std::this_thread:sleep_for(std::chrono::seconds(1));
}
```

Эта программа выведет «!»:

```c++
int main()
{
    BgThreadDispatcher dispatcher;
    dispatcher.Wait(); // Фоновых задач нет, метод сразу вернёт управление
    std::cout << "!";  // Выведет ! в главном потоке.
}
```

Эта программа с большой вероятностью выведет «1!» и с низкой «!»:

```c++
int main()
{
    BgThreadDispatcher dispatcher;
    dispatcher.Dispatch([]{ osync(std::cout) << "1"; }); // Выведет 1 в фоновом потоке
    // 1 секунды, скорее всего, хватит, чтобы успела вывестись 1.
    std::this_thread:sleep_for(std::chrono::seconds(1));
    dispatcher.Stop(); // Останавливаем выполнение фоновых задач
    dispatcher.Dispatch([]{ osync(std::cout) << "2"; }); // 2 выведена не будет, так как раньше был вызван Stop
    dispatcher.Wait(); // Дожидаемся завершения фоновых задач
    std::cout << "!";  // Выведет ! в главном потоке.
}
```

#### Подсказки

- Используйте `std::jthread` внутри `BgThreadDispatcher` для запуска фонового потока.
- Вам может понадобиться несколько экземпляров `std::conditional_variable`,
  чтобы организовать обмен информацией между потоками.
- Кроме `std::vector` есть и другие контейнеры.
- В юнит-тестах проверьте все описанные в примерах сценарии и при необходимости добавьте свои.
- Возможно, вы найдёте полезное применение классам `std::stop_token`, `std::stop_source`, `std::stop_callback` и `std::atomic<>`.
- Старайтесь не держать блокировку мьютекса дольше, чем необходимо.
  Что точно не стоит делать, так это вызывать `Task` с захваченным мьютексом.
- Ваш фреймворк для юнит-тестирования может иметь ограничения на работу с многопоточным кодом.
  Ознакомьтесь с инструкцией.
- Написание корректного многопоточного кода — сложная работа, требующая внимания и терпения. Не торопитесь.

### Задание 3 — Пул потоков — 150 баллов

Это задание — усовершенствованная версия предыдущей задачи. В нём нужно разработать класс `ThreadPool`,
который создаёт заданное количество рабочих потоков, которые параллельно извлекают задачи из очереди
и выполняют их.

```c++
class ThreadPool
{
public:
    using Task = std::function<void()>;

    // Создаёт пул из numThreads рабочих потоков
    explicit ThreadPool(unsigned numThreads);

    // Копирование и присваивание объектов этого класса невозможно
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Завершает работу фонового потока. 
    ~ThreadPool();

    // Отправляет задачу task на выполнение в любом из фоновых потоков внутри пула.
    // Этот метод можно вызывать из любого потока, в том числе из любого потока внутри пула.
    // Если ThreadPool остановлен, задача игнорируется.
    void Dispatch(Task task);

    // Блокирует работу текущего потока до окончания выполнения всех фоновых задач.
    // Вызов этого метода из фонового потока приводит к неопределённому поведению.
    // Если ранее был вызван метод Stop, то ожидание завершения не происходит.
    void Wait();

    // Сообщает рабочим потокам о необходимости остановить работу
    // Задачи, которые не успели выполниться, игнорируются.
    void Stop();
};
```

Для тестирования класса `ThreadPool` разработайте юнит-тесты.

### Задание 4

## Ссылки

- Класс [`std::mutex`](https://en.cppreference.com/w/cpp/thread/mutex)
- Класс [`std::shared_mutex`](https://en.cppreference.com/w/cpp/thread/shared_mutex)
- Класс [`std::unique_lock`](https://en.cppreference.com/w/cpp/thread/unique_lock)
- Класс [`std::shared_lock`](https://en.cppreference.com/w/cpp/thread/shared_lock)
- Класс [`std::lock_guard`](https://en.cppreference.com/w/cpp/thread/lock_guard)
- Класс [`std::conditional_variable`](https://en.cppreference.com/w/cpp/thread/condition_variable)
- Класс [`std::stop_callback`](https://en.cppreference.com/w/cpp/thread/stop_callback)