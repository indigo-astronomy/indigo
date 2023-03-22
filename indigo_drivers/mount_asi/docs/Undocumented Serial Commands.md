# ZWO AM5 Undocumented Commands
## Altitude limits
### **:SLLnn#**
Set low altitude limit (0-30&deg;).
Returns "1" on success. Example: :SLL20#

### **:GLL#**
Get low altitude limit (0-30&deg;).
Returns **nn#** in degrees

### **:SLHnn#**
Set hight altitude limit (60-90&deg;).
Returns "1" on success. Example: :SLH80#

### **:GLH#**
Get high altitude limit (60-90&deg;).
Returns **nn#** in degrees

### **:SLE#**
Enable altitude limitis.
Returns "1" on success.

### **:SLD#**
Disable altitude limitis.
Returns "1" on success.

### **:GLC#**
Get altitude limits status:
**0#** - limits disabled, **1#** - limits enabled

## Slew speeds
### **:SRlnnnn#**
Set slew sppeed like: 720, 1440.
Returns "1" on success.  Example :SRl1440#

### **:GRl#**
Get slew sppeed.
Returns **nnnn#** speed like: 720 or 1440

## Unknown
### **:GFD1#**
No idea what it does - ASCOM driver issues it with :GR# and :GD# commands every second or so.
Returns **22438#**

### **:GFR1#**
No idea what it does - ASCOM driver issues it with :GR# and :GD# commands every second or so.
Returns **22438#**

Both GFR1 and GFD1 always return the same value **22438#**

## Open questions
Needs to be tested what happens if invalid data is provided like: **:SLL45#**. Most likely wrong input will result in **eX#** resonance where X is error code or "0" for failure.
