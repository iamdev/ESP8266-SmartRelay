//***********************************************************************************
// MIT License
// Copyright (c) 2019 Kamon Singtong
//***********************************************************************************
// Library : MovingAverage-Ring-Buffer
// Virsion : 0.1
// Owner : Kamon Singtong (MakeArduino.com)
// email : kamon.dev@hotmail.com
// fb : makearduino
//***********************************************************************************

#ifndef _MV_RING_BUFFER_H
#define _MV_RING_BUFFER_H
template <class T>
class MVRingBuffer{
  public :
    MVRingBuffer<T>(int buf_size=10);    
    MVRingBuffer<T>(T *buffer,int buf_size);
    void add(T v);
    double average() const {return _average;}
    T Min(void) const {return _minValue;}
    T Max(void) const {return _maxValue;}
    T first();
    T last();
    T operator[] (const int index);
  private:
    T _minValue;
    T _maxValue;
    T _average;
    T _sumValue;        
    T getAt(const int index);
    T *_buffer;
    int _index;
    int buffer_size; 
};

template <class T> inline 
MVRingBuffer<T>::MVRingBuffer(int buf_size):buffer_size(buf_size)
{
  _index =0;
  _buffer = (T*) malloc (sizeof(T)*buffer_size+1);
}

template <class T> inline 
MVRingBuffer<T>::MVRingBuffer(T *buffer,int buf_size):buffer_size(buf_size)
{
  _index =0;
  _buffer = buffer;
}

template <class T> inline 
void MVRingBuffer<T>::add(T v){
  _sumValue += v - _buffer[_index];
  _buffer[_index] = v;
  _index ++;
  _average = _sumValue/buffer_size;
  if(_minValue == _minValue == 0){
      _minValue = v;
      _maxValue = v;
  }else{
      if(v < _minValue) _minValue = v;
      if(v > _maxValue) _maxValue = v;
  }
  if(_index>=buffer_size)_index=0;
}

template <class T> inline 
T MVRingBuffer<T>::first(){
    return getAt(buffer_size-1);
}

template <class T> inline 
T MVRingBuffer<T>::last(){
    return getAt(0);
}

template <class T> inline 
T MVRingBuffer<T>::getAt (const int index)
{
  return _buffer[(index+_index) % buffer_size];

}

template <class T> inline 
T MVRingBuffer<T>::operator[] (const int index)
{
    return getAt(index);
}

#endif /* _MV_RING_BUFFER*/
