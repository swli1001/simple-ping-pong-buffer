#include <systemc.h>

class write_if : virtual public sc_interface
{
  public:
    virtual void write(char) = 0;
    virtual void let_end() = 0;
    virtual bool get_write() = 0;
};

class read_if : virtual public sc_interface
{
  public:
    virtual void read(char &) = 0;
    virtual bool get_char() = 0;
    virtual bool get_end() = 0;
};

class fifo : public sc_channel, public write_if, public read_if
{
  public:
    fifo(sc_module_name name) : sc_channel(name), num_elements1(0), first1(0), num_elements2(0), first2(0), d1_flag(true), d2_flag(true), w_id(true), r_id(true), end(false)  {}
    // d1_flag, d2_flag = true: the data buffer is empty, false: buffer is full and read available
    // w_id, r_id = true: now should write/read data1[], false: should write/read data2[]     
    void write(char c) {
      written = false;
      if (w_id && d1_flag) {
        cout << sc_time_stamp() << ": <W1> " << c << endl;
        data1[(first1 + num_elements1) % max] = c;
        ++ num_elements1;
        if (num_elements1 == max) { w_id = false; d1_flag = false; }
        written = true;
      }

      if (!written && d2_flag && !w_id) {
        cout << sc_time_stamp() << ": <W2> " << c << endl;
        data2[(first2 + num_elements2) % max] = c;
        ++ num_elements2;
        if (num_elements2 == max) { w_id = true; d2_flag = false; }
      }
    }

    void read(char &c) {
      read_val = false;
      if (end && num_elements1 > 0) { d1_flag = false; }
      if (r_id && !d1_flag) {
        read_val = true;
        c = data1[first1];
        cout << "\t\t" << sc_time_stamp() << ": <R1> " << flush;
        -- num_elements1;
        first1 = (first1 + 1) % max;
        if (num_elements1 == 0) { d1_flag = true; r_id = false; }
      }

      if (end && num_elements2 > 0) { d2_flag = false; }
      if (!read_val && !d2_flag && !r_id) {
        read_val = true;
        c = data2[first2];
        cout << "\t\t" << sc_time_stamp() << ": <R2> " << flush;
        -- num_elements2;
        first2 = (first2 + 1) % max;
        if (num_elements2 == 0) { d2_flag = true; r_id = true; }
      }
    }

    bool get_write() { return d1_flag || d2_flag; }

    bool get_char() { return read_val; }

    void let_end() { end = true; }
    bool get_end() {
      if (end && num_elements1 == 0 && num_elements2 == 0){ return true; }
      else { return false; }
    }

  private:
    enum e { max = 10 };
    char data1[max], data2[max];
    int num_elements1, first1;
    int num_elements2, first2;
    bool d1_flag, d2_flag;
    bool w_id, r_id;
    bool written, read_val;
    bool end;
};

class producer : public sc_module
{
  public:
    sc_port<write_if> out;
    //sc_port<sc_signal_in_if<bool> > clk; sensitive << clk;
    sc_in_clk clk;

    SC_HAS_PROCESS(producer);

    producer(sc_module_name name) : sc_module(name)
    {
      SC_THREAD(main);
      sensitive << clk.pos();
    }

    void main()
    {
      wait(SC_ZERO_TIME);
      const char *str = "Visit www.accellera.org and see what SystemC can do for you today!\n";

      while(*str){
        if (out->get_write())
          out->write(*str++);
        wait();
      }
      out->let_end();
    }
};

class consumer : public sc_module
{
  public:
    sc_port<read_if> in;
    //sc_port<sc_signal_in_if<bool> > clk;
    sc_in_clk clk;

    SC_HAS_PROCESS(consumer);

    consumer(sc_module_name name) : sc_module(name)
    {
      SC_THREAD(main);
      sensitive << clk.pos();
    }

    void main()
    {
      char c;
      cout << endl << endl;

      while(true){
        in->read(c);
        if (in->get_char()){
          cout << c << endl;
        }
        if (in->get_end()){ sc_stop(); }
        wait();
      }
    }
};

int sc_main(int, char *[]){
  fifo *fifo_inst;
  producer *prod_inst;
  consumer *cons_inst;
  sc_clock clk_w("clk_w", 2, SC_SEC, 0.5, 0, SC_SEC, true);
  sc_clock clk_r("clk_r", 1, SC_SEC, 0.5, 0, SC_SEC, true);

  fifo_inst = new fifo("Fifo1");

  prod_inst = new producer("Producer1");
  prod_inst->out(*fifo_inst);
  prod_inst->clk(clk_w);

  cons_inst = new consumer("Consumer1");
  cons_inst->in(*fifo_inst);
  cons_inst->clk(clk_r);

  sc_start();
  return 0;
}
