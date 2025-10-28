const SNOWFLAKE_COUNT = 110;
const snowflakes = [];
let windowWidth, windowHeight;

const isWinter = () => {
    const monthIndex = new Date().getMonth() + 1; 
    return monthIndex >= 10 || monthIndex <= 2;
};

const handleResize = () => {
    windowWidth = window.innerWidth;
    windowHeight = window.innerHeight;
};

const createSnowflake = (container) => {
    const xPosition = Math.random() * windowWidth;
    const yPosition = Math.random() * (windowHeight + 50) - (windowHeight + 50);
    const ySpeed = Math.random() * 1.5 + 0.5;

    const opacity =  Math.random() * 0.3 + 0.2;
    const fontsize = Math.random() * 2 + 1.5;

    const driftMagnitude = Math.random() * 30 + 10;
    const driftFrequency = Math.random() * 0.0005 + 0.0001;
    const driftOffset = Math.random() * 2 * Math.PI;

    container.appendChild(Object.assign(document.createElement('div'), {
        className: 'snowflake',
        innerText: '*',
        style: `font-size:${fontsize}rem; opacity:${opacity};`
    }));

    snowflakes.push({
        element: container.lastChild, 
        xPosition, 
        yPosition, 
        ySpeed, 
        initialX: xPosition,
        driftMagnitude,
        driftFrequency,
        driftOffset,
    });
};

const updateSnowflakes = (timestamp) => {
    const timeFactor = timestamp * 0.001; 

    snowflakes.forEach(flake => {
        let { yPosition, ySpeed, 
            xPosition, initialX, 
            driftFrequency, driftOffset,
            driftMagnitude, element 
        } = flake;

        yPosition += ySpeed;
        xPosition = initialX + Math.sin(
            timeFactor * driftFrequency + driftOffset
        ) * driftMagnitude;
        
        if (yPosition > windowHeight + 50) {
            yPosition = -20;
            xPosition = initialX = Math.random() * windowWidth; 
        }

        Object.assign(flake, { 
            yPosition, xPosition, initialX 
        });

        element.style.transform = `translate3d(${xPosition}px, ${yPosition}px, 0)`;
    });

    requestAnimationFrame(updateSnowflakes);
};

window.onload = () => {
    if (!isWinter()) return;

    const containerElement = Object.assign(
        document.createElement('div'), { 
            id: 'snow-container' 
    });
    document.body.prepend(containerElement);

    handleResize();
    window.addEventListener('resize', handleResize);
    
    for (let i = 0; i < SNOWFLAKE_COUNT; i++) {
        createSnowflake(containerElement);
    }
    updateSnowflakes(Date.now());
};
