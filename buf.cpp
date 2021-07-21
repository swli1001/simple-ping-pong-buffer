#include <systemc.h>

class write_if : virtual public sc_interface
{
  public:
    virtual void write(char) = 0;
    virtual void let_end() = 0;
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
    fifo(sc_module_name name) : sc_channel(name), num_elements1(0), first1(0), num_elements2(0), first2(0), cw1(true), cw2(false), cr1(false), cr2(false), read_val(false), end(false), written(false) {}
    
    void write(char c) {
      written = false;
      if (cw1) {
        cout << sc_time_stamp() << ": <W1> " << c << endl;
        data1[(first1 + num_elements1) % max] = c;
        ++ num_elements1;
        if (num_elements1 == max) { cw1 = false; cw2 = true; cr1 = true; }
        written = true;
      }
      
      if (cw2 && !written) {
        cout << sc_time_stamp() << ": <W2> " << c << endl;
        data2[(first2 + num_elements2) % max] = c;
        ++ num_elements2;
        if (num_elements2 == max) {  cw2 = false; cw1 = true; cr2 = true; }
      }
    }

    void read(char &c) {
      read_val = false;
      if (!cr1 && end && num_elements2 == 0) { cr1 = true; }
      if (cr1) {
        read_val = true;
        c = data1[first1];
        cout << "\t\t" << sc_time_stamp() << ": <R1> " << flush;
        -- num_elements1;
        first1 = (first1 + 1) % max;
        if (num_elements1 == 0) { cr1 = false; }
      }
      if (!cr2 && end && num_elements1 == 0) { cr2 = true; } 
      if (cr2 && !read_val) {
        read_val = true;
        c = data2[first2];
        cout << "\t\t" << sc_time_stamp() << ": <R2> " << flush;
        -- num_elements2;
        first2 = (first2 + 1) % max;
        if (num_elements2 == 0) { cr2 = false; }
      }
    }

    bool get_char(){ return read_val; }
    
    void let_end(){ end = true; }
    bool get_end(){
      if (end && num_elements1 == 0 && num_elements2 == 0){ return true; }
      else { return false; }
    }

  private:
    enum e { max = 10 }; 
    char data1[max], data2[max];
    int num_elements1, first1;
    int num_elements2, first2;
    bool cw1, cw2, cr1, cr2;
    bool read_val;
    bool end;
    bool written, have_read;
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
  sc_clock clk("clk", 1, SC_SEC, 0.5, 0, SC_SEC, true);

  fifo_inst = new fifo("Fifo1");
  
  prod_inst = new producer("Producer1");
  prod_inst->out(*fifo_inst);
  prod_inst->clk(clk);

  cons_inst = new consumer("Consumer1");
  cons_inst->in(*fifo_inst);
  cons_inst->clk(clk);  

  sc_start();
  return 0;
}
