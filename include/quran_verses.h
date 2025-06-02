// quran_verses.h
#ifndef QURAN_VERSES_H
#define QURAN_VERSES_H

struct QuranVerse {
    const char* arabic;
    const char* translation;
    const char* reference;
};

// Array of daily verses - will rotate through these
const QuranVerse DAILY_VERSES[] = {
    {
        "وَأَقِمِ الصَّلَاةَ طَرَفَيِ النَّهَارِ وَزُلَفًا مِّنَ اللَّيْلِ",
        "And establish prayer at the two ends of the day and at the approach of the night",
        "Hud 11:114"
    },
    {
        "إِنَّ الصَّلَاةَ كَانَتْ عَلَى الْمُؤْمِنِينَ كِتَابًا مَّوْقُوتًا",
        "Indeed, prayer has been decreed upon the believers at specified times",
        "An-Nisa 4:103"
    },
    {
        "وَاسْتَعِينُوا بِالصَّبْرِ وَالصَّلَاةِ",
        "And seek help through patience and prayer",
        "Al-Baqarah 2:45"
    },
    {
        "الَّذِينَ هُمْ عَلَىٰ صَلَاتِهِمْ دَائِمُونَ",
        "Those who are constant in their prayer",
        "Al-Ma'arij 70:23"
    },
    {
        "حَافِظُوا عَلَى الصَّلَوَاتِ وَالصَّلَاةِ الْوُسْطَىٰ",
        "Maintain with care the prayers and the middle prayer",
        "Al-Baqarah 2:238"
    },
    {
        "فَسَبِّحْ بِحَمْدِ رَبِّكَ وَكُن مِّنَ السَّاجِدِينَ",
        "So glorify your Lord with praise and be among those who prostrate",
        "Al-Hijr 15:98"
    },
    {
        "رَبَّنَا تَقَبَّلْ مِنَّا ۖ إِنَّكَ أَنتَ السَّمِيعُ الْعَلِيمُ",
        "Our Lord, accept this from us. Indeed, You are the All-Hearing, the All-Knowing",
        "Al-Baqarah 2:127"
    },
    {
        "وَإِذَا سَأَلَكَ عِبَادِي عَنِّي فَإِنِّي قَرِيبٌ",
        "And when My servants ask you concerning Me, indeed I am near",
        "Al-Baqarah 2:186"
    },
    {
        "اللَّهُ لَطِيفٌ بِعِبَادِهِ يَرْزُقُ مَن يَشَاءُ ۖ وَهُوَ الْقَوِيُّ الْعَزِيزُ",
        "Allah is Subtle with His servants; He gives provisions to whom He wills. And He is the Powerful, the Exalted in Might",
        "Ash-Shura 42:19"
    },
    {
        "يَا أَيُّهَا الَّذِينَ آمَنُوا اذْكُرُوا اللَّهَ ذِكْرًا كَثِيرًا",
        "O you who believe, remember Allah with much remembrance",
        "Al-Ahzab 33:41"
    },
    {
        "وَمَا تَوْفِيقِي إِلَّا بِاللَّهِ ۚ عَلَيْهِ تَوَكَّلْتُ وَإِلَيْهِ أُنِيبُ",
        "And my success is not but through Allah. Upon Him I have relied, and to Him I return",
        "Hud 11:88"
    },
    {
        "فَاذْكُرُونِي أَذْكُرْكُمْ وَاشْكُرُوا لِي وَلَا تَكْفُرُونِ",
        "So remember Me; I will remember you. And be grateful to Me and do not deny Me",
        "Al-Baqarah 2:152"
    },
    {
        "قُلْ إِنَّ صَلَاتِي وَنُسُكِي وَمَحْيَايَ وَمَمَاتِي لِلَّهِ رَبِّ الْعَالَمِينَ",
        "Say, Indeed, my prayer, my rites of sacrifice, my living and my dying are for Allah, Lord of the worlds",
        "Al-An'am 6:162"
    },
    {
        "الَّذِينَ آمَنُوا وَتَطْمَئِنُّ قُلُوبُهُم بِذِكْرِ اللَّهِ ۗ أَلَا بِذِكْرِ اللَّهِ تَطْمَئِنُّ الْقُلُوبُ",
        "Those who believe and whose hearts find rest in the remembrance of Allah. Truly in the remembrance of Allah do hearts find rest",
        "Ar-Ra'd 13:28"
    },
    {
        "رَبَّنَا آتِنَا فِي الدُّنْيَا حَسَنَةً وَفِي الْآخِرَةِ حَسَنَةً وَقِنَا عَذَابَ النَّارِ",
        "Our Lord, grant us good in this world and good in the Hereafter, and protect us from the punishment of the Fire",
        "Al-Baqarah 2:201"
    },
    {
        "وَاصْبِرْ لِحُكْمِ رَبِّكَ فَإِنَّكَ بِأَعْيُنِنَا",
        "And be patient for the decision of your Lord, for indeed, you are in Our eyes",
        "At-Tur 52:48"
    },
    {
        "إِنَّ مَعَ الْعُسْرِ يُسْرًا",
        "Indeed, with hardship will be ease",
        "Ash-Sharh 94:6"
    },
    {
        "وَقُل رَّبِّ زِدْنِي عِلْمًا",
        "And say, 'My Lord, increase me in knowledge'",
        "Ta-Ha 20:114"
    },
    {
        "وَإِذْ تَأَذَّنَ رَبُّكُمْ لَئِن شَكَرْتُمْ لَأَزِيدَنَّكُمْ",
        "And when your Lord proclaimed, 'If you are grateful, I will surely increase you'",
        "Ibrahim 14:7"
    },
    {
        "اللَّهُ نُورُ السَّمَاوَاتِ وَالْأَرْضِ",
        "Allah is the Light of the heavens and the earth",
        "An-Nur 24:35"
    }
};

const int NUM_VERSES = sizeof(DAILY_VERSES) / sizeof(DAILY_VERSES[0]);

#endif // QURAN_VERSES_H