/** @type {import('tailwindcss').Config} */
export default {
    content: [
        "./index.html",
        "./src/**/*.{js,ts,jsx,tsx}",
    ],
    darkMode: 'class',
    theme: {
        extend: {
            colors: {
                background: '#09090b', // zinc-950
                surface: '#18181b', // zinc-900
                border: '#27272a', // zinc-800
                primary: '#3b82f6', // blue-500
                nvm: {
                    free: '#22c55e', // green-500
                    used: '#3b82f6', // blue-500
                    shadow: '#a855f7', // purple-500
                    leaked: '#ef4444', // red-500
                }
            }
        },
    },
    plugins: [],
}
