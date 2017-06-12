#include <df.h>

unsigned int lastpage=0; //last page written to
unsigned int pages=8; //total pages that will be used
df dflash; 

void setup()
{
  Serial.begin(9600);
  Serial.print('h');
  Serial.print('i');
  Serial.print('\n');//debug
  dflash.init(); //initialize the memory (pins are defined in dataflash.cpp
}

void loop()
{
  int j = 0;
  int i = 0;
  char messageline[] = "spltech pvt limited vikas nagar uttamnagar delhi west";
  //unsigned char message[5];
  char lpstring[] = "lastpage: ";
  char int_string[10];
  //unsigned char * xd=messageline;

  itoa(lastpage, int_string, 10); // make string of the pagenumber
//  strcat(messageline, int_string); //append to the messagline string
  //for (int i=0; messageline[i] != '\0'; i++)
  
//  dflash.Page_Erase (lastpage);
//dflash.Main_Write_Str (1, lastpage, sizeof(messageline), messageline);

//dflash.Update_str(1, lastpage, sizeof(messageline), messageline);

  /*
  while (messageline[i] != '\0')
  {
    Serial.print(messageline[i]); //just a check to see if the loop is working
   //commented by asim
   dflash.Buffer_Write_Byte(1, i, messageline[i]);  //write to buffer 1, 1 byte at the time
   //dflash.Main_Write_Byte (lastpage, i, messageline[i]);
    j = i;
    i++;
    delay(2);
  }
  i=0;
  //below commented by asim
  dflash.Buffer_Write_Byte(1, j+1, '\0'); //terminate the string in the buffer
  Serial.print('\t');
  dflash.Buffer_To_Page(1, lastpage); //write the buffer to the memory on page: lastpage

  strcat(lpstring, int_string);
  for(int i=0; lpstring[i] != '\0';i++)
  {
    dflash.Buffer_Write_Byte(2, 20, lpstring[i]); //write to buffer 2 the lastpage number that was used

    Serial.print(lpstring[i]); //write to serial port the lastpage number written to
  }
  */
  Serial.println();
  lastpage++;
  if (lastpage > pages) //if we reach the end of the range of pages
  {
    lastpage = 0;
    for (unsigned int i=0;i<=pages;i++)
    {
     dflash.Page_To_Buffer(i, 1);//copy page i to the buffer

      for(int j=0;j<32;j++) //32 depends on the amount of data on the page
                            // testing for a specific charater is also possible
      {
       // Serial.print(dflash.Buffer_Read_Byte(1, j)); //print the buffer data to the serial port
       //  Serial.print(dflash.Buffer_Read_Byte(1, j));  //print the buffer data to the serial port
      //Serial.print(dflash.Buffer_Read_Byte(1, j),HEX);  //print the buffer data to the serial port
          Serial.write(dflash.Buffer_Read_Byte(1, j));  //print the buffer data to the serial port
      }
      Serial.print("  page: "); 
      Serial.println(i); //print the last read page number
    }
  }
  delay(500); //slow it down a bit, just for easier reading
}
