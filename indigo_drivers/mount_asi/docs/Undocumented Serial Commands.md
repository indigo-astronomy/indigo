# ZWO AM5 Undocumented Commands

## Altitude limits
Limits are preserved after power cycle.

### **:SLLnn#**
Set low altitude limit (0-30&deg;).
Returns "1" on success, "0" on error. Example: :SLL20#

### **:GLL#**
Get low altitude limit (0-30&deg;).
Returns **nn#** in degrees

### **:SLHnn#**
Set hight altitude limit (60-90&deg;).
Returns "1" on success, "0" on error. Example: :SLH80#

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


## Clear alignment data
Each sync adds alignment point. They can only be removed all at once.

### **:NSC#**
Clear alignment data (remove all alignment points).
Returns "1" on success.


## Slew speed
Slew speed is preserved after power cycle.

### **:SRlnnnn#**
Set slew speed: 720 or 1440. (720 is Low and 1440 is High)
Returns "1" on success, "0" on error.  Example :SRl1440# or :SRl720#

### **:GRl#**
Get slew speed.
Returns **nnnn#** speed: 720 or 1440

## Unknown
### **:GFD1#**
No idea what it does - ASCOM driver issues it with :GR# and :GD# commands every second or so.
Returns **22438#**

### **:GFR1#**
No idea what it does - ASCOM driver issues it with :GR# and :GD# commands every second or so.
Returns **22438#**

Both GFR1 and GFD1 always return the same value **22438#**
