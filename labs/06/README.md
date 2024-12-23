# Лабораторная работа №6

- [Лабораторная работа №6](#лабораторная-работа-6)
  - [Задания](#задания)
    - [Требования](#требования)
    - [Задание 1 — Менеджер памяти — 120 баллов](#задание-1--менеджер-памяти--120-баллов)
      - [Бонус: сделать менеджер памяти потокобезопасным — 30 баллов](#бонус-сделать-менеджер-памяти-потокобезопасным--30-баллов)
    - [Задание 2 — Работа с отображением файлов в память в Linux — 70 баллов](#задание-2--работа-с-отображением-файлов-в-память-в-linux--70-баллов)
      - [Подсказки](#подсказки)
    - [Задание 3 — Работа с отображением файлов в память в Windows — 150 баллов](#задание-3--работа-с-отображением-файлов-в-память-в-windows--150-баллов)

## Задания

- Для получения оценки "удовлетворительно" нужно набрать не менее 100 баллов.
- Для получения оценки "хорошо" нужно набрать не менее 200 баллов.
- Для получения оценки "отлично" нужно набрать не менее 300 баллов.

### Требования

Обязательно проверяйте успешность всех вызовов функций операционной системы и не оставляйте ошибки незамеченными.

Ваш код должен иметь уровень безопасности исключений не ниже базового.
Для этого разработайте (или возьмите готовую) RAII-обёртку, автоматизирующую
управление ресурсами операционной системы.

### Задание 1 — Менеджер памяти — 120 баллов

Разработайте класс `MemoryManager`, позволяющий динамически выделять и освобождать память
внутри блока памяти, переданного ему в конструктор.

Для класса разработайте необходимый набор юнит-тестов.

```c++
#include <cassert>
#include <memory>
#include <utility>

class MemoryManager
{
public:
    // Инициализирует менеджер памяти непрерывным блоком size байт,
    // начиная с адреса start.
    // Возвращает true в случае успеха и false в случае ошибки
    // Методы Allocate и Free должны работать с этим блоком памяти для хранения данных.
    // Указатель start должен быть выровнен по адресу, кратному sizeof(std::max_align_t)
    MemoryManager(void* start, size_t size) noexcept
    {
    }

    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    // Выделяет блок памяти внутри размером size байт и возвращает адрес выделенного
    // блока памяти. Возвращённый указатель должен быть выровнен по адресу, кратному align.
    // Параметр align должен быть степенью числа 2.
    // В случае ошибки (нехватка памяти, невалидные параметры) возвращает nullptr.
    // Полученный таким образом блок памяти должен быть позднее освобождён методом Free
    void* Allocate(size_t size, size_t align = sizeof(std::max_align_t)) noexcept
    {
        return nullptr;
    }

    // Освобождает область памяти, ранее выделенную методом Allocate,
    // делая её пригодной для повторного использования. После этого указатель
    // перестаёт быть валидным.
    // Если addr — нулевой указатель, метод не делает ничего
    // Если addr — не является валидным указателем, возвращённым ранее методом Allocate,
    // поведение метода не определено.
    void Free(void* addr) noexcept
    {
    }
};

int main()
{
    alignas(sizeof(max_align_t)) char buffer[1000];

    MemoryManager memoryManager(buffer, std::size(buffer));

    auto ptr = memoryManager.Allocate(sizeof(double));

    auto value = std::construct_at(static_cast<double*>(ptr), 3.1415927);
    assert(*value == 3.1415927);
        
    memoryManager.Free(ptr);
}
```

Класс должен использовать O(1) памяти в дополнение к переданному в конструктор блоку памяти.

#### Бонус: сделать менеджер памяти потокобезопасным — 30 баллов

Методы класса `MemoryManager` должно быть возможно безопасно вызывать из нескольких потоков одновременно.
Разработайте модульные тесты для проверки этого функционала.

### Задание 2 — Работа с отображением файлов в память в Linux — 70 баллов

Ознакомьтесь с работой функций [`mmap`, `munmap`](https://man7.org/linux/man-pages/man2/mmap.2.html) и
[`mremap`](https://man7.org/linux/man-pages/man2/mremap.2.html) в Linux.

Разработайте программу, которая строит гистограмму частоты встречаемости символов в файле.
Путь к файлу передаётся программе через командную строку. В stdout программа должна вывести
256 целых чисел (по одному в строке), равных частоте встречаемости соответствующих байтов.

Подсчёт частоты встречаемости символов должен выполняться параллельно в нескольких потоках.
Придумайте, как распределить объем работы между потоками, чтобы обеспечить максимально возможное
ускорение по сравнению с однопоточным вариантом.
Рекомендуется использовать пул потоков из предыдущей лабораторной работы, либо взять готовый.

Размер файла может быть больше доступного объема физической памяти.
Поэтому целиком загрузить его в память не получится. Загружать его в буфер по частям — выполнять двойную работу.
Эффективнее отобразить часть файла прямо в память компьютера и работать с ним в памяти напрямую.
Для этого используйте функции для отображения файлов в память.

Имя файла и количество потоков передаются через командную строку:

```sh
hystogram FILE_NAME NUM_THREADS
```

Постройте диаграмму, показывающую зависимость времени работы программы от количества потоков для файлов разного размера:

- 1 гигабайт
- 10 гигабайт
- 50 гигабайт

#### Подсказки

- Чтобы потоки не блокировались на мьютексе и не сериализовали доступ к памяти на атомарных переменных,
  каждый из них должен строить частичную гистограмму для своей части файла.
  Частичные гистограммы в конце должны объединяться в одну итоговую гистограмму.
- Корректно обработайте ситуации, когда размер файла не кратен размеру страницы.

**Допускается решить эту задачу под Windows при условии выполнения задачи №3 под Linux**.

### Задание 3 — Работа с отображением файлов в память в Windows — 150 баллов

Ознакомьтесь с [отображаемыми в память файлами](https://learn.microsoft.com/en-us/windows/win32/memory/file-mapping)
в Windows
Разработайте программу, которая обрабатывает **двоичные** файлы, содержащие записи типа `Person` вида:

```c++
struct Date
{
  uint16_t date;
  uint8_t day;
  uint8_t month;
};

struct Person
{
  Date birthDate;     // Дата рождения
  uint8_t nameLength; // Длина имени
  char name[59];      // Массив символов имени без завершающего нулевого символа
};
// Размер записи, равный степени числа 2 гарантирует, что записи
// не будут пересекать границы страниц памяти.
static_assert(sizeof(Person) == 64);
```

Программа должна работать в двух режимах:

- Генерирование файла. Генерирует файл со случайно заполненными именами и датами рождения.
  Имена должны составляться путём случайной выборки из массивов мужских и женских имён и фамилий.
  Даты должны быть равномерно распределены по годам от 1900 до 2024
- Сортировка записей в файле. Записи должны сортироваться сперва по неубыванию даты рождения, а потом
  по неубыванию имени.

Синтаксис командной строки для генерирования файла:

```sh
sorter generate FILE_NAME NUM_RECORDS NUM_THREADS
```

```sh
sorter sort FILE_NAME NUM_THREADS
```

Количество записей в файле таково, что в физическую память он целиком не поместится.

И генерирование, и сортировка файла должны выполняться в многопоточном режиме, путём отображения
файлов в память. Так вы сможете работать с частями файла как с массивом данных в памяти,
а операционная система сама позаботится о том, чтобы синхронизировать содержимое файла и памяти.

Для многопоточного генерирования файла разделите файл на несколько участков и
распределите эти участки между рабочими потоками.

Чтобы не делить между потоками один и тот же генератор случайных чисел
и не синхронизировать доступ к нему с помощью мьютекса,
каждый из потоков должен использовать отдельный генератор,
инициализированный уникальным начальным значением seed.

Чтобы не создавать временные строки при сравнении имён, используйте `std::string_view`.

Объединение блоков сортировкой слиянием может становится сложнее, когда файл не помещается
в физическую память целиком. Подумайте, как доработать сортировку слиянием, в таких условиях.

Чтобы продемонстрировать работу программы с условиях ограниченной памяти,
ограничьте объём отображаемой памяти, который разрешается использовать каждому потоку в один момент времени,
4 мегабайтами. Плюс дополнительно каждому потоку разрешается использовать не более 8 мегабайт в куче
(учтите, что `std::inplace_merge` может выделить дополнительный буфер, чтобы выполнить сортировку за $O(N)$ сравнений).
В таких условиях программа должна быть способна отсортировать файл размером несколько сотен мегабайт используя
не более двух рабочих потоков.

Постройте диаграмму зависимости времени генерирования файла и сортировки
в зависимости от количества потоков для файлов размера:

- $2^{20}$ записей (64 Мегабайта)
- $10*2^{20}$ записей (640 Мегабайт)
- $100*2^{20}$ записей (6,4 Гигабайта)

При замерах времени сортировки учтите, что после сортировки файл становится отсортирован,
и каждый раз нужно сортировать изначально сгенерированный файл.

**Допускается решить эту задачу под Linux при условии выполнения задачи №2 под Widnows**.
