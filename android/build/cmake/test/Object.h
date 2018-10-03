class MyClass : public QObject {
    Q_OBJECT
    Q_PROPERTY(Priority priority READ priority WRITE setPriority)
    Q_ENUMS(Priority)

public:
    enum Priority { High, Low, VeryHigh, VeryLow };

    MyClass(QObject* parent = 0);
    ~MyClass();

    void setPriority(Priority priority) { m_priority = priority; }
    Priority priority() const { return m_priority; }

private:
    Priority m_priority;
};
