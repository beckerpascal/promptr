using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PromptrLogic
{
    public interface IPromptrClient
    {
        void TurnOn();
        void TurnOn(int id);
        void TurnOff();
        void TurnOff(int id);
    }
}
