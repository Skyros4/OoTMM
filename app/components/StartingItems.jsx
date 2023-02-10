import React from 'react';

export const StartingItems = ({ settings, setSetting, itemPool }) => {
  const alterItem = (item, delta) => {
    const { startingItems } = settings;
    const count = startingItems[item] || 0;
    if (count + delta < 0) {
      delete startingItems[item];
    } else {
      startingItems[item] = count + delta;
      if (startingItems[item] > itemPool[item]) {
        startingItems[item] = itemPool[item];
      }
    }
    setSetting({ startingItems });
  };

  const resetItems = () => {
    setSetting({ startingItems: {}})
  };

  // Valid gamePrefix are "MM" and "OOT"
  const buildSingleTable = (gamePrefix) => {
    return (
      <table>
        <thead>
          <tr>
            <th colspan="2">{gamePrefix === "MM" ? "Majora's Mask" : "Ocarina of Time"}</th>
          </tr>
        </thead>
        <tbody>
          {Object.keys(itemPool)
            .filter(item => item.startsWith(gamePrefix))
            .map(item =>
              <tr key={item} className={settings.startingItems[item] > 0 ? "active" : "inactive"}>
                <td className="count">
                  <button className="count-adjust" onClick={() => alterItem(item, -1)}>-</button>
                  {settings.startingItems[item] || 0}
                  <button className="count-adjust" onClick={() => alterItem(item, 1)}>+</button>
                </td>
                <td>{item}</td>
              </tr>
            )}
        </tbody>
      </table>
    )
  }

  return (
    <React.Fragment>
      <button className="reset-button" onClick={() => resetItems()}>Reset Starting Items</button>
      <div className="starting-items">
        {buildSingleTable("MM")}
        {buildSingleTable("OOT")}
      </div>
    </React.Fragment>
  );
};
