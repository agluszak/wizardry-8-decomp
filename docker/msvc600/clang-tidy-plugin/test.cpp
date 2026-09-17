void make(double, double, int, int);
void overloaded(float);
void overloaded(double);
void format(const char*, ...);

struct Sink {
    void take(double);
};

void positives(long width, float scale, int numerator, int denominator, Sink& sink)
{
    make((double)((float)(int)width * scale), (double)scale, 1, 1);
    double ratio = (double)numerator / (double)denominator;
    sink.take((double)scale);
    format("%f", (double)scale);
    (void)ratio;
}

template <class T> T authored_template(T value)
{
    if ((double)value != 0.0) {
        return (T)(1.0 / value);
    }
    return (T)0;
}

void negatives(unsigned count, double precise, float value)
{
    double ratio = static_cast<double>(count) / 3;
    float narrowed = static_cast<float>(precise);
    overloaded(static_cast<double>(1.0f));
    float templated = authored_template(value);
    (void)ratio;
    (void)narrowed;
    (void)templated;
}
